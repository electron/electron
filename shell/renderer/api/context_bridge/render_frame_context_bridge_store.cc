// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/render_frame_context_bridge_store.h"

#include <utility>

#include "shell/common/api/remote/object_life_monitor.h"

namespace electron {

namespace api {

namespace context_bridge {

namespace {

class CachedProxyLifeMonitor final : public ObjectLifeMonitor {
 public:
  static void BindTo(v8::Isolate* isolate,
                     v8::Local<v8::Object> target,
                     base::WeakPtr<RenderFramePersistenceStore> store,
                     WeakGlobalPairNode* node,
                     int hash) {
    new CachedProxyLifeMonitor(isolate, target, store, node, hash);
  }

 protected:
  CachedProxyLifeMonitor(v8::Isolate* isolate,
                         v8::Local<v8::Object> target,
                         base::WeakPtr<RenderFramePersistenceStore> store,
                         WeakGlobalPairNode* node,
                         int hash)
      : ObjectLifeMonitor(isolate, target),
        store_(store),
        node_(node),
        hash_(hash) {}

  void RunDestructor() override {
    if (!store_)
      return;

    if (node_->detached) {
      delete node_;
      return;
    }
    if (node_->prev) {
      node_->prev->next = node_->next;
      node_->prev = nullptr;
    }
    if (node_->next) {
      node_->next->prev = node_->prev;
      node_->next = nullptr;
    }
    if (!node_->prev && !node_->next) {
      // Must be a single length linked list
      store_->proxy_map()->erase(hash_);
    }
    node_->detached = true;
  }

 private:
  base::WeakPtr<RenderFramePersistenceStore> store_;
  WeakGlobalPairNode* node_;
  int hash_;
};

}  // namespace

std::map<int32_t, RenderFramePersistenceStore*>& GetStoreMap() {
  static base::NoDestructor<std::map<int32_t, RenderFramePersistenceStore*>>
      store_map;
  return *store_map;
}

WeakGlobalPairNode::WeakGlobalPairNode(WeakGlobalPair pair) {
  this->pair = std::move(pair);
}

WeakGlobalPairNode::~WeakGlobalPairNode() {}

RenderFramePersistenceStore::RenderFramePersistenceStore(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      routing_id_(render_frame->GetRoutingID()) {}

RenderFramePersistenceStore::~RenderFramePersistenceStore() = default;

void RenderFramePersistenceStore::OnDestruct() {
  GetStoreMap().erase(routing_id_);
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
    CachedProxyLifeMonitor::BindTo(v8::Isolate::GetCurrent(), obj,
                                   weak_factory_.GetWeakPtr(), node, hash);
    CachedProxyLifeMonitor::BindTo(v8::Isolate::GetCurrent(),
                                   v8::Local<v8::Object>::Cast(proxy_value),
                                   weak_factory_.GetWeakPtr(), node, hash);
    if (iter == proxy_map_.end()) {
      proxy_map_.emplace(hash, node);
    } else {
      WeakGlobalPairNode* target = iter->second;
      while (target->next) {
        target = target->next;
      }
      target->next = node;
      node->prev = target;
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
