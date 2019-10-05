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

class RenderFramePersistenceStore final : public content::RenderFrameObserver {
 public:
  explicit RenderFramePersistenceStore(content::RenderFrame* render_frame);
  ~RenderFramePersistenceStore() override;

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  size_t take_func_id() { return next_func_id_++; }

  std::map<size_t, FunctionContextPair>& functions() { return functions_; }
  std::map<int, v8::Global<v8::Value>>* main_world_proxy_map() {
    return &main_world_proxy_map_;
  }
  std::map<int, v8::Global<v8::Value>>* isolated_world_proxy_map() {
    return &isolated_world_proxy_map_;
  }

  v8::Local<v8::Context> isolated_world_context() {
    CHECK(render_frame());
    auto* frame = render_frame()->GetWebFrame();
    CHECK(frame);
    return frame->WorldScriptContext(v8::Isolate::GetCurrent(),
                                     World::ISOLATED_WORLD);
  }

  v8::Local<v8::Context> main_world_context() {
    CHECK(render_frame());
    auto* frame = render_frame()->GetWebFrame();
    CHECK(frame);
    return frame->MainWorldScriptContext();
  }

  void CacheProxiedObject(v8::Local<v8::Value> from,
                          v8::Local<v8::Value> proxy_value);
  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(v8::Local<v8::Value> from);

 private:
  std::map<int, v8::Global<v8::Value>>* GetProxyMapForObject(
      v8::Local<v8::Object> obj);

  // func_id ==> { function, owning_context }
  std::map<size_t, FunctionContextPair> functions_;
  size_t next_func_id_ = 1;

  // proxy maps are weak globals, i.e. these are not retained beyond
  // there normal JS lifetime.  You must check IsEmpty()

  // object_identity (main world) ==> proxy_value (isolated world)
  std::map<int, v8::Global<v8::Value>> main_world_proxy_map_;
  // object_identity (isolated world) ==> proxy_value (main world)
  std::map<int, v8::Global<v8::Value>> isolated_world_proxy_map_;
};

}  // namespace context_bridge

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_CONTEXT_BRIDGE_STORE_H_
