// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/object_cache.h"

#include "shell/common/api/object_life_monitor.h"

namespace electron::api::context_bridge {

ObjectCache::ObjectCache() = default;
ObjectCache::~ObjectCache() = default;

void ObjectCache::CacheProxiedObject(v8::Local<v8::Value> from,
                                     v8::Local<v8::Value> proxy_value) {
  if (from->IsObject() && !from->IsNullOrUndefined()) {
    auto obj = from.As<v8::Object>();
    int hash = obj->GetIdentityHash();

    proxy_map_[hash].emplace_front(from, proxy_value);
  }
}

v8::MaybeLocal<v8::Value> ObjectCache::GetCachedProxiedObject(
    v8::Local<v8::Value> from) const {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return v8::MaybeLocal<v8::Value>();

  auto obj = from.As<v8::Object>();
  int hash = obj->GetIdentityHash();
  auto iter = proxy_map_.find(hash);
  if (iter == proxy_map_.end())
    return v8::MaybeLocal<v8::Value>();

  auto& list = iter->second;
  for (const auto& pair : list) {
    auto from_cmp = pair.first;
    if (from_cmp == from) {
      if (pair.second.IsEmpty())
        return v8::MaybeLocal<v8::Value>();
      return pair.second;
    }
  }
  return v8::MaybeLocal<v8::Value>();
}

}  // namespace electron::api::context_bridge
