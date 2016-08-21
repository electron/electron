// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
#define ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_

#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_frame.h"

namespace atom {

class AtomSandboxedRendererClient : public content::ContentRendererClient {
 public:
  AtomSandboxedRendererClient();
  virtual ~AtomSandboxedRendererClient();

  void DidCreateScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame);
  void WillReleaseScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame);
  void InvokeBindingCallback(v8::Handle<v8::Context> context,
                             std::string callback_name,
                             std::vector<v8::Handle<v8::Value>> args);
  // content::ContentRendererClient:
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomSandboxedRendererClient);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_SANDBOXED_RENDERER_CLIENT_H_
