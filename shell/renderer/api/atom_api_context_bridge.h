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
#include "native_mate/dictionary.h"
#include "shell/common/node_includes.h"

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

  size_t take_id() { return next_id_++; }

  std::map<size_t, FunctionContextPair>& functions() { return functions_; }

 private:
  // func_id ==> { function, owning_context }
  std::map<size_t, FunctionContextPair> functions_;

  size_t next_id_ = 1;
};

v8::Local<v8::Value> ProxyFunctionWrapper(RenderFramePersistenceStore* store,
                                          size_t func_id,
                                          mate::Arguments* args);

mate::Dictionary CreateProxyForAPI(mate::Dictionary api,
                                   v8::Local<v8::Context> source_context,
                                   v8::Local<v8::Context> target_context,
                                   RenderFramePersistenceStore* store);

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_
