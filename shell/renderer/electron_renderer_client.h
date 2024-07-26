// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "shell/renderer/renderer_client_base.h"

namespace node {
class Environment;
}

namespace electron {

class ElectronBindings;
class NodeBindings;

class ElectronRendererClient : public RendererClientBase {
 public:
  ElectronRendererClient();
  ~ElectronRendererClient() override;

  // disable copy
  ElectronRendererClient(const ElectronRendererClient&) = delete;
  ElectronRendererClient& operator=(const ElectronRendererClient&) = delete;

  // electron::RendererClientBase:
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                content::RenderFrame* render_frame) override;

 private:
  void UndeferLoad(content::RenderFrame* render_frame);

  // content::ContentRendererClient:
  void RenderFrameCreated(content::RenderFrame*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void WorkerScriptReadyForEvaluationOnWorkerThread(
      v8::Local<v8::Context> context) override;
  void WillDestroyWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;

  node::Environment* GetEnvironment(content::RenderFrame* frame) const;

  // Whether the node integration has been initialized.
  bool node_integration_initialized_ = false;

  const std::unique_ptr<NodeBindings> node_bindings_;
  const std::unique_ptr<ElectronBindings> electron_bindings_;

  // The node::Environment::GetCurrent API does not return nullptr when it
  // is called for a context without node::Environment, so we have to keep
  // a book of the environments created.
  base::flat_set<std::shared_ptr<node::Environment>> environments_;

  // Getting main script context from web frame would lazily initializes
  // its script context. Doing so in a web page without scripts would trigger
  // assertion, so we have to keep a book of injected web frames.
  base::flat_set<content::RenderFrame*> injected_frames_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_
