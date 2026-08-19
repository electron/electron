// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_

#include <memory>

#include "base/containers/flat_map.h"
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
  void DidCreateScriptContext(v8::Isolate* isolate,
                              v8::Local<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Isolate* isolate,
                                v8::Local<v8::Context> context,
                                content::RenderFrame* render_frame) override;

 private:
  void UndeferLoad(content::RenderFrame* render_frame);

  // content::ContentRendererClient:
  void PostIOThreadCreated(
      base::SingleThreadTaskRunner* io_thread_task_runner) override;
  void RenderFrameCreated(content::RenderFrame*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void WorkerScriptReadyForEvaluationOnWorkerThread(
      v8::Local<v8::Context> context) override;
  void WillDestroyWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;
  void SetUpWebAssemblyTrapHandler() override;

  // A frame's node::Environment and the uv loop integration driving it. Every
  // environment gets its own loop so that freeing one, which runs its loop
  // until its handles close, never dispatches another environment's callbacks.
  struct FrameEnvironment;

  node::Environment* GetEnvironment(content::RenderFrame* frame) const;

  // Whether the node integration has been initialized.
  bool node_integration_initialized_ = false;

  // Integrates uv_default_loop() for the whole process and hosts a main
  // frame's environment while one exists; subframes never borrow it.
  const std::unique_ptr<NodeBindings> node_bindings_;
  const std::unique_ptr<ElectronBindings> electron_bindings_;

  base::flat_map<content::RenderFrame*, std::unique_ptr<FrameEnvironment>>
      environments_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_RENDERER_CLIENT_H_
