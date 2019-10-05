// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/render_frame_context_bridge_store.h"

namespace electron {

namespace api {

namespace context_bridge {

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
    auto global_proxy =
        v8::Global<v8::Value>(v8::Isolate::GetCurrent(), proxy_value);
    // Do not retain
    global_proxy.SetWeak();
    GetProxyMapForObject(obj)->emplace(hash, std::move(global_proxy));
  }
}

v8::MaybeLocal<v8::Value> RenderFramePersistenceStore::GetCachedProxiedObject(
    v8::Local<v8::Value> from) {
  if (!from->IsObject() || from->IsNullOrUndefined())
    return v8::MaybeLocal<v8::Value>();

  auto obj = v8::Local<v8::Object>::Cast(from);
  int hash = obj->GetIdentityHash();
  auto* map = GetProxyMapForObject(obj);
  auto iter = map->find(hash);
  if (iter == map->end() || iter->second.IsEmpty())
    return v8::MaybeLocal<v8::Value>();
  return v8::MaybeLocal<v8::Value>(iter->second.Get(v8::Isolate::GetCurrent()));
}

std::map<int, v8::Global<v8::Value>>*
RenderFramePersistenceStore::GetProxyMapForObject(v8::Local<v8::Object> obj) {
  v8::Local<v8::Context> from_context =
      v8::Local<v8::Object>::Cast(obj)->CreationContext();
  v8::Local<v8::Context> main_context = main_world_context();
  v8::Local<v8::Context> isolated_context = isolated_world_context();
  bool is_main_context = from_context == main_context;
  CHECK(is_main_context || from_context == isolated_context);
  if (is_main_context) {
    return &main_world_proxy_map_;
  } else {
    return &isolated_world_proxy_map_;
  }
}

}  // namespace context_bridge

}  // namespace api

}  // namespace electron
