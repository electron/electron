// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/object_cache.h"

#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-object.h"

namespace electron::api::context_bridge {

ObjectCache::ObjectCache() = default;
ObjectCache::~ObjectCache() = default;

void ObjectCache::CacheProxiedObject(const v8::Local<v8::Value> from,
                                     v8::Local<v8::Value> proxy_value) {
  if (from->IsObject() && !from->IsNullOrUndefined())
    proxy_map_.insert_or_assign(from.As<v8::Object>(), proxy_value);
}

v8::MaybeLocal<v8::Value> ObjectCache::GetCachedProxiedObject(
    const v8::Local<v8::Value> from) const {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return {};

  const auto iter = proxy_map_.find(from.As<v8::Object>());
  if (iter == proxy_map_.end() || iter->second.IsEmpty())
    return {};

  return iter->second;
}

}  // namespace electron::api::context_bridge
