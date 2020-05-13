// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/object_cache.h"

#include <utility>

#include "shell/common/api/object_life_monitor.h"

namespace electron {

namespace api {

namespace context_bridge {

ObjectCachePairNode::ObjectCachePairNode(ObjectCachePair&& pair) {
  this->pair = std::move(pair);
}

ObjectCachePairNode::~ObjectCachePairNode() = default;

ObjectCache::ObjectCache() {}
ObjectCache::~ObjectCache() = default;

void ObjectCache::CacheProxiedObject(v8::Local<v8::Value> from,
                                     v8::Local<v8::Value> proxy_value) {
  if (from->IsObject() && !from->IsNullOrUndefined()) {
    auto obj = v8::Local<v8::Object>::Cast(from);
    int hash = obj->GetIdentityHash();

    auto* node = new ObjectCachePairNode(std::make_pair(from, proxy_value));
    proxy_map_[hash].Append(node);
  }
}

v8::MaybeLocal<v8::Value> ObjectCache::GetCachedProxiedObject(
    v8::Local<v8::Value> from) const {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return v8::MaybeLocal<v8::Value>();

  auto obj = v8::Local<v8::Object>::Cast(from);
  int hash = obj->GetIdentityHash();
  auto iter = proxy_map_.find(hash);
  if (iter == proxy_map_.end())
    return v8::MaybeLocal<v8::Value>();

  auto& list = iter->second;
  for (auto* node = list.head(); node != list.end(); node = node->next()) {
    auto& pair = node->value()->pair;
    auto from_cmp = pair.first;
    if (from_cmp == from) {
      if (pair.second.IsEmpty())
        return v8::MaybeLocal<v8::Value>();
      return pair.second;
    }
  }
  return v8::MaybeLocal<v8::Value>();
}

}  // namespace context_bridge

}  // namespace api

}  // namespace electron
