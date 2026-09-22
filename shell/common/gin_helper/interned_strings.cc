// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/interned_strings.h"

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "gin/converter.h"
#include "gin/per_isolate_data.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-primitive.h"

namespace gin_helper {

namespace {

// One cache per thread, holding entries for one isolate at a time; seeing a
// different isolate drops what came before.
class InternedStringCache final : public gin::PerIsolateData::DisposeObserver {
 public:
  InternedStringCache() = default;
  ~InternedStringCache() override { Detach(); }

  InternedStringCache(const InternedStringCache&) = delete;
  InternedStringCache& operator=(const InternedStringCache&) = delete;

  v8::Local<v8::String> Get(v8::Isolate* isolate, std::string_view literal) {
    if (isolate_ != isolate) [[unlikely]] {
      Detach();
      auto* const data = gin::PerIsolateData::From(isolate);
      if (!data) {
        // A Node.js worker's isolate has no PerIsolateData and never reports
        // disposal, so a cached handle could outlive it. Don't cache.
        return gin::StringToSymbol(isolate, literal);
      }
      isolate_ = isolate;
      data_ = data;
      data_->AddDisposeObserver(this);
    }
    auto [it, inserted] = strings_.try_emplace(literal.data());
    if (inserted)
      it->second.Set(isolate, gin::StringToSymbol(isolate, literal));
    return it->second.Get(isolate);
  }

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override { strings_.clear(); }
  void OnBeforeMicrotasksRunnerDispose(v8::Isolate* isolate) override {}
  void OnDisposed() override { Detach(); }

 private:
  void Detach() {
    if (data_)
      data_->RemoveDisposeObserver(this);
    strings_.clear();
    isolate_ = nullptr;
    data_ = nullptr;
  }

  raw_ptr<v8::Isolate> isolate_ = nullptr;
  raw_ptr<gin::PerIsolateData> data_ = nullptr;
  // Keyed by the literal's address; see the header.
  absl::flat_hash_map<const char*, v8::Eternal<v8::String>> strings_;
};

InternedStringCache& CacheForThisThread() {
  // Never destroyed: it is a registered dispose observer, and the entries are
  // released when the isolate reports disposal anyway.
  thread_local base::NoDestructor<InternedStringCache> cache;
  return *cache;
}

}  // namespace

namespace internal {

v8::Local<v8::String> InternedStringImpl(v8::Isolate* isolate,
                                         std::string_view literal) {
  return CacheForThisThread().Get(isolate, literal);
}

}  // namespace internal

}  // namespace gin_helper
