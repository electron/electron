// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
#define ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_

#include <memory>
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
  void InvokeIpcCallback(v8::Handle<v8::Context> context,
                         const std::string& callback_name,
                         std::vector<v8::Handle<v8::Value>> args);
  // atom::RendererClientBase:
  void DidCreateScriptContext(v8::Handle<v8::Context> context,
                              content::RenderFrame* render_frame) override;
  void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                content::RenderFrame* render_frame) override;
  void SetupMainWorldOverrides(v8::Handle<v8::Context> context,
                               content::RenderFrame* render_frame) override;
  // content::ContentRendererClient:
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;

 private:
  std::unique_ptr<base::ProcessMetrics> metrics_;

  DISALLOW_COPY_AND_ASSIGN(AtomSandboxedRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
