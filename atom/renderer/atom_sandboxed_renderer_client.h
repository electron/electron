// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
#define ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "atom/renderer/renderer_client_base.h"
#include "base/process/process_metrics.h"

namespace atom {

class AtomSandboxedRendererClient : public RendererClientBase {
 public:
  AtomSandboxedRendererClient();
  ~AtomSandboxedRendererClient() override;

  void InitializeBindings(v8::Local<v8::Object> binding,
                          v8::Local<v8::Context> context,
                          bool is_main_frame);
  // atom::RendererClientBase:
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                content::RenderFrame* render_frame) override;
  void SetupMainWorldOverrides(v8::Handle<v8::Context> context,
                               content::RenderFrame* render_frame) override;
  void SetupExtensionWorldOverrides(v8::Handle<v8::Context> context,
                                    content::RenderFrame* render_frame,
                                    int world_id) override;
  // content::ContentRendererClient:
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;
  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;

 private:
  std::unique_ptr<base::ProcessMetrics> metrics_;

  // Getting main script context from web frame would lazily initializes
  // its script context. Doing so in a web page without scripts would trigger
  // assertion, so we have to keep a book of injected web frames.
  std::set<content::RenderFrame*> injected_frames_;

  DISALLOW_COPY_AND_ASSIGN(AtomSandboxedRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
