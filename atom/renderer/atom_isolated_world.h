// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_ISOLATED_WORLD_H_
#define ATOM_RENDERER_ATOM_ISOLATED_WORLD_H_

#include "atom/common/node_bindings.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"

namespace atom {

class AtomIsolatedWorld : public v8::Extension {
 public:
  explicit AtomIsolatedWorld(NodeBindings* node_bindings);
  ~AtomIsolatedWorld() override;
  node::Environment* CreateEnvironment(content::RenderFrame* frame);
  v8::Local<v8::FunctionTemplate> GetNativeFunctionTemplate(
      v8::Isolate* isolate,
      v8::Local<v8::String> name) override;

  static AtomIsolatedWorld* Create(NodeBindings* node_bindings);

 private:
  static void SetupNode(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  static NodeBindings* node_bindings_;
  static node::Environment* env_;
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_ISOLATED_WORLD_H_
