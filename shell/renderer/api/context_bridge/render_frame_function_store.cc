// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/context_bridge/render_frame_function_store.h"

#include <utility>

#include "shell/common/api/remote/object_life_monitor.h"

namespace electron {

namespace api {

namespace context_bridge {

std::map<int32_t, RenderFrameFunctionStore*>& GetStoreMap() {
  static base::NoDestructor<std::map<int32_t, RenderFrameFunctionStore*>>
      store_map;
  return *store_map;
}

RenderFrameFunctionStore::RenderFrameFunctionStore(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      routing_id_(render_frame->GetRoutingID()) {}

RenderFrameFunctionStore::~RenderFrameFunctionStore() = default;

void RenderFrameFunctionStore::OnDestruct() {
  GetStoreMap().erase(routing_id_);
  delete this;
}

void RenderFrameFunctionStore::WillReleaseScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  base::EraseIf(functions_, [context](auto const& pair) {
    v8::Local<v8::Context> func_owning_context =
        std::get<1>(pair.second).Get(context->GetIsolate());
    return func_owning_context == context;
  });
}

}  // namespace context_bridge

}  // namespace api

}  // namespace electron
