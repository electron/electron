// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_CONTEXT_BRIDGE_STORE_H_
#define SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_CONTEXT_BRIDGE_STORE_H_

#include <map>
#include <tuple>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/renderer/atom_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace context_bridge {

using FunctionContextPair =
    std::tuple<v8::Global<v8::Function>, v8::Global<v8::Context>>;

using WeakGlobalPair = std::tuple<v8::Global<v8::Value>, v8::Global<v8::Value>>;

struct WeakGlobalPairNode {
  explicit WeakGlobalPairNode(WeakGlobalPair pair_);
  ~WeakGlobalPairNode();
  WeakGlobalPair pair;
  bool detached = false;
  struct WeakGlobalPairNode* prev = nullptr;
  struct WeakGlobalPairNode* next = nullptr;
};

class RenderFramePersistenceStore final : public content::RenderFrameObserver {
 public:
  explicit RenderFramePersistenceStore(content::RenderFrame* render_frame);
  ~RenderFramePersistenceStore() override;

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  size_t take_func_id() { return next_func_id_++; }

  std::map<size_t, FunctionContextPair>& functions() { return functions_; }
  std::map<int, WeakGlobalPairNode*>* proxy_map() { return &proxy_map_; }

  void CacheProxiedObject(v8::Local<v8::Value> from,
                          v8::Local<v8::Value> proxy_value);
  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(v8::Local<v8::Value> from);

 private:
  // func_id ==> { function, owning_context }
  std::map<size_t, FunctionContextPair> functions_;
  size_t next_func_id_ = 1;

  // proxy maps are weak globals, i.e. these are not retained beyond
  // there normal JS lifetime.  You must check IsEmpty()

  const int32_t routing_id_;

  // object_identity ==> [from_value, proxy_value]
  std::map<int, WeakGlobalPairNode*> proxy_map_;
  base::WeakPtrFactory<RenderFramePersistenceStore> weak_factory_{this};
};

std::map<int32_t, RenderFramePersistenceStore*>& GetStoreMap();

}  // namespace context_bridge

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_CONTEXT_BRIDGE_STORE_H_
