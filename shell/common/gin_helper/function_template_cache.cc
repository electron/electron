// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/function_template_cache.h"

#include "base/memory/raw_ptr.h"
#include "base/no_destructor.h"
#include "gin/per_isolate_data.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper {

namespace {

class FunctionTemplateCache final
    : public gin::PerIsolateData::DisposeObserver {
 public:
  FunctionTemplateCache() = default;
  ~FunctionTemplateCache() override { Detach(); }

  FunctionTemplateCache(const FunctionTemplateCache&) = delete;
  FunctionTemplateCache& operator=(const FunctionTemplateCache&) = delete;

  v8::Local<v8::FunctionTemplate> Get(v8::Isolate* isolate, const void* key) {
    if (!Attach(isolate))
      return {};
    auto it = templates_.find(key);
    if (it == templates_.end())
      return {};
    return it->second.Get(isolate);
  }

  void Set(v8::Isolate* isolate,
           const void* key,
           v8::Local<v8::FunctionTemplate> tmpl) {
    if (!Attach(isolate))
      return;
    // An Eternal can only be set once, so replace the entry.
    templates_.erase(key);
    templates_[key].Set(isolate, tmpl);
  }

  // gin::PerIsolateData::DisposeObserver
  void OnBeforeDispose(v8::Isolate* isolate) override { Detach(); }
  void OnDisposed() override {}

 private:
  bool Attach(v8::Isolate* isolate) {
    if (isolate_ == isolate)
      return true;
    auto* const data = gin::PerIsolateData::From(isolate);
    if (!data)
      return false;
    Detach();
    isolate_ = isolate;
    data_ = data;
    data_->AddDisposeObserver(this);
    return true;
  }

  void Detach() {
    if (data_)
      data_->RemoveDisposeObserver(this);
    templates_.clear();
    isolate_ = nullptr;
    data_ = nullptr;
  }

  raw_ptr<v8::Isolate> isolate_ = nullptr;
  raw_ptr<gin::PerIsolateData> data_ = nullptr;
  absl::flat_hash_map<const void*, v8::Eternal<v8::FunctionTemplate>>
      templates_;
};

FunctionTemplateCache& CacheForThisThread() {
  thread_local base::NoDestructor<FunctionTemplateCache> cache;
  return *cache;
}

}  // namespace

v8::Local<v8::FunctionTemplate> GetCachedFunctionTemplate(v8::Isolate* isolate,
                                                          const void* key) {
  return CacheForThisThread().Get(isolate, key);
}

void SetCachedFunctionTemplate(v8::Isolate* isolate,
                               const void* key,
                               v8::Local<v8::FunctionTemplate> tmpl) {
  CacheForThisThread().Set(isolate, key, tmpl);
}

}  // namespace gin_helper
