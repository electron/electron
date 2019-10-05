// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_
#define SHELL_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_

#include <map>
#include <string>
#include <tuple>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/renderer/atom_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

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

  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(v8::Local<v8::Value> from) {
    if (!from->IsObject() || from->IsNullOrUndefined())
      return v8::MaybeLocal<v8::Value>();

    auto obj = v8::Local<v8::Object>::Cast(from);
    int hash = obj->GetIdentityHash();
    auto* map = GetProxyMapForObject(obj);
    auto iter = map->find(hash);
    if (iter == map->end() || iter->second.IsEmpty())
      return v8::MaybeLocal<v8::Value>();
    return v8::MaybeLocal<v8::Value>(
        iter->second.Get(v8::Isolate::GetCurrent()));
  }

 private:
  std::map<int, v8::Global<v8::Value>>* GetProxyMapForObject(
      v8::Local<v8::Object> obj) {
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

v8::Local<v8::Value> ProxyFunctionWrapper(RenderFramePersistenceStore* store,
                                          size_t func_id,
                                          mate::Arguments* args);

v8::Local<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source,
    v8::Local<v8::Context> destination,
    v8::Local<v8::Value> value,
    RenderFramePersistenceStore* store);

mate::Dictionary CreateProxyForAPI(mate::Dictionary api,
                                   v8::Local<v8::Context> source_context,
                                   v8::Local<v8::Context> target_context,
                                   RenderFramePersistenceStore* store);

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_
