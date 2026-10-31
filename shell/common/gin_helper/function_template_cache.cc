// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/function_template_cache.h"

#include <cstdint>
#include <memory>

#include "base/check_op.h"
#include "base/no_destructor.h"
#include "base/threading/thread_local.h"
#include "gin/per_isolate_data.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper {

namespace {

// A thread only ever runs one isolate that has gin::PerIsolateData: the main
// thread of the browser, utility and renderer processes or of node_main, or a
// Blink worker thread, which exits after disposing its isolate. Node.js workers
// have none and are never cached.
// Entries are v8::Eternal handles, so they need no V8 teardown, but they
// outlive the isolate and must not be read once it is gone.
struct ThreadCache {
  absl::flat_hash_map<const void*, v8::Eternal<v8::FunctionTemplate>> templates;
  // Only used to catch a second isolate on this thread, which would leave
  // stale entries behind.
  uintptr_t isolate_address = 0;
};

ThreadCache& CacheForThisThread(v8::Isolate* isolate) {
  // Freed when the thread exits. A plain thread_local would need a trivially
  // destructible type and leak the map on every thread that fills it.
  static base::NoDestructor<base::ThreadLocalOwnedPointer<ThreadCache>> tls;
  if (!tls->Get()) {
    tls->Set(std::make_unique<ThreadCache>());
  }
  ThreadCache& cache = *tls->Get();
  const auto address = reinterpret_cast<uintptr_t>(isolate);
  if (!cache.isolate_address) {
    cache.isolate_address = address;
  }
  DCHECK_EQ(cache.isolate_address, address)
      << "A thread must not run more than one isolate";
  return cache;
}

bool IsCacheable(v8::Isolate* isolate) {
  return gin::PerIsolateData::From(isolate) != nullptr;
}

}  // namespace

v8::Local<v8::FunctionTemplate> GetCachedFunctionTemplate(v8::Isolate* isolate,
                                                          const void* key) {
  if (!IsCacheable(isolate))
    return {};
  auto& templates = CacheForThisThread(isolate).templates;
  auto it = templates.find(key);
  return it == templates.end() ? v8::Local<v8::FunctionTemplate>()
                               : it->second.Get(isolate);
}

void SetCachedFunctionTemplate(v8::Isolate* isolate,
                               const void* key,
                               v8::Local<v8::FunctionTemplate> tmpl) {
  if (!IsCacheable(isolate))
    return;
  // An Eternal occupies a slot until the isolate is destroyed, so
  // never replace an entry.
  auto [it, inserted] = CacheForThisThread(isolate).templates.try_emplace(key);
  DCHECK(inserted) << "A cached function template must not be replaced";
  if (inserted)
    it->second.Set(isolate, tmpl);
}

}  // namespace gin_helper
