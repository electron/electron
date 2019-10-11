// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/render_frame_context_bridge_store.h"

#include <utility>

namespace electron {

namespace api {

namespace context_bridge {

WeakGlobalPairNode::WeakGlobalPairNode(WeakGlobalPair pair_) {
  pair = std::move(pair_);
}

WeakGlobalPairNode::~WeakGlobalPairNode() {
  if (next) {
    delete next;
  }
}

RenderFramePersistenceStore::RenderFramePersistenceStore(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

RenderFramePersistenceStore::~RenderFramePersistenceStore() = default;

void RenderFramePersistenceStore::OnDestruct() {
  delete this;
}

void RenderFramePersistenceStore::CacheProxiedObject(
    v8::Local<v8::Value> from,
    v8::Local<v8::Value> proxy_value) {
  if (from->IsObject() && !from->IsNullOrUndefined()) {
    auto obj = v8::Local<v8::Object>::Cast(from);
    int hash = obj->GetIdentityHash();
    auto global_from = v8::Global<v8::Value>(v8::Isolate::GetCurrent(), from);
    auto global_proxy =
        v8::Global<v8::Value>(v8::Isolate::GetCurrent(), proxy_value);
    // Do not retain
    global_from.SetWeak();
    global_proxy.SetWeak();
    auto iter = proxy_map_.find(hash);
    auto* node = new WeakGlobalPairNode(
        std::make_tuple(std::move(global_from), std::move(global_proxy)));
    if (iter == proxy_map_.end()) {
      proxy_map_.emplace(hash, node);
    } else {
      WeakGlobalPairNode* target = iter->second;
      while (target->next) {
        target = target->next;
      }
      target->next = node;
    }
  }
}

v8::MaybeLocal<v8::Value> RenderFramePersistenceStore::GetCachedProxiedObject(
    v8::Local<v8::Value> from) {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return v8::MaybeLocal<v8::Value>();

  auto obj = v8::Local<v8::Object>::Cast(from);
  int hash = obj->GetIdentityHash();
  auto iter = proxy_map_.find(hash);
  if (iter == proxy_map_.end())
    return v8::MaybeLocal<v8::Value>();
  WeakGlobalPairNode* target = iter->second;
  while (target) {
    auto from_cmp = std::get<0>(target->pair).Get(v8::Isolate::GetCurrent());
    if (from_cmp == from) {
      if (std::get<1>(target->pair).IsEmpty())
        return v8::MaybeLocal<v8::Value>();
      return std::get<1>(target->pair).Get(v8::Isolate::GetCurrent());
    }
    target = target->next;
  }
  return v8::MaybeLocal<v8::Value>();
}

}  // namespace context_bridge

}  // namespace api

}  // namespace electron
