// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_FUNCTION_STORE_H_
#define SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_FUNCTION_STORE_H_

#include <map>
#include <tuple>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace context_bridge {

using FunctionContextPair =
    std::tuple<v8::Global<v8::Function>, v8::Global<v8::Context>>;

class RenderFrameFunctionStore final : public content::RenderFrameObserver {
 public:
  explicit RenderFrameFunctionStore(content::RenderFrame* render_frame);
  ~RenderFrameFunctionStore() override;

  // RenderFrameObserver implementation.
  void OnDestruct() override;

  size_t take_func_id() { return next_func_id_++; }

  std::map<size_t, FunctionContextPair>& functions() { return functions_; }

  base::WeakPtr<RenderFrameFunctionStore> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  // func_id ==> { function, owning_context }
  std::map<size_t, FunctionContextPair> functions_;
  size_t next_func_id_ = 1;

  const int32_t routing_id_;

  base::WeakPtrFactory<RenderFrameFunctionStore> weak_factory_{this};
};

std::map<int32_t, RenderFrameFunctionStore*>& GetStoreMap();

}  // namespace context_bridge

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_CONTEXT_BRIDGE_RENDER_FRAME_FUNCTION_STORE_H_
