// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_RENDERER_CLIENT_BASE_H_
#define ATOM_RENDERER_RENDERER_CLIENT_BASE_H_

#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"

namespace atom {

class PreferencesManager;

class RendererClientBase : public content::ContentRendererClient {
 public:
  RendererClientBase();
  virtual ~RendererClientBase();

  virtual void DidCreateScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame) = 0;
  virtual void WillReleaseScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame) = 0;
  virtual void DidClearWindowObject(content::RenderFrame* render_frame);
  virtual void SetupMainWorldOverrides(v8::Handle<v8::Context> context) = 0;
  virtual bool isolated_world() = 0;

 protected:
  void AddRenderBindings(v8::Isolate* isolate,
                         v8::Local<v8::Object> binding_object);

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;
  blink::WebSpeechSynthesizer* OverrideSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            blink::WebLocalFrame* frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const std::string& mime_type,
      const GURL& original_url) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;

 private:
  std::unique_ptr<PreferencesManager> preferences_manager_;
};

}  // namespace atom

#endif  // ATOM_RENDERER_RENDERER_CLIENT_BASE_H_
