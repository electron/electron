// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef ELECTRON_SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_
#define ELECTRON_SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_

#include <memory>
#include <set>

#include "shell/renderer/renderer_client_base.h"

namespace base {
class ProcessMetrics;
}

namespace blink {
class WebLocalFrame;
}

namespace electron {

class ElectronSandboxedRendererClient : public RendererClientBase {
 public:
  ElectronSandboxedRendererClient();
  ~ElectronSandboxedRendererClient() override;

  // disable copy
  ElectronSandboxedRendererClient(const ElectronSandboxedRendererClient&) =
      delete;
  ElectronSandboxedRendererClient& operator=(
      const ElectronSandboxedRendererClient&) = delete;

  void InitializeBindings(v8::Local<v8::Object> binding,
                          v8::Local<v8::Context> context,
                          content::RenderFrame* render_frame);
  // electron::RendererClientBase:
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                content::RenderFrame* render_frame) override;
  // content::ContentRendererClient:
  void RenderFrameCreated(content::RenderFrame*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void WillEvaluateServiceWorkerOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      v8::Local<v8::Context> v8_context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url,
      const blink::ServiceWorkerToken& service_worker_token) override;
  void WillDestroyServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;

 private:
  void EmitProcessEvent(content::RenderFrame* render_frame,
                        const char* event_name);

  std::unique_ptr<base::ProcessMetrics> metrics_;

  // Getting main script context from web frame would lazily initializes
  // its script context. Doing so in a web page without scripts would trigger
  // assertion, so we have to keep a book of injected web frames.
  std::set<content::RenderFrame*> injected_frames_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_ELECTRON_SANDBOXED_RENDERER_CLIENT_H_
