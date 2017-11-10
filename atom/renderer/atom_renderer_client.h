// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_
#define ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_

#include <string>
#include <vector>

#include "atom/renderer/renderer_client_base.h"

namespace atom {

class AtomBindings;
class NodeBindings;

class AtomRendererClient : public RendererClientBase {
 public:
  AtomRendererClient();
  virtual ~AtomRendererClient();

  // atom::RendererClientBase:
  void DidCreateScriptContext(
      v8::Handle<v8::Context> context,
      content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(
      v8::Handle<v8::Context> context,
      content::RenderFrame* render_frame) override;
  void SetupMainWorldOverrides(v8::Handle<v8::Context> context) override;

 private:
  enum NodeIntegration {
    ALL,
    EXCEPT_IFRAME,
    MANUAL_ENABLE_IFRAME,
    DISABLE,
  };

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  bool ShouldFork(blink::WebLocalFrame* frame,
                  const GURL& url,
                  const std::string& http_method,
                  bool is_initial_navigation,
                  bool is_server_redirect,
                  bool* send_referrer) override;
  void DidInitializeWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;
  void WillDestroyWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context) override;

  // Whether the node integration has been initialized.
  bool node_integration_initialized_;

  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<AtomBindings> atom_bindings_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDERER_CLIENT_H_
