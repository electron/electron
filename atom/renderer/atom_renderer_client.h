// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"

namespace atom {

class AtomBindings;
class PreferencesManager;
class NodeBindings;

class AtomRendererClient : public content::ContentRendererClient {
 public:
  AtomRendererClient();
  virtual ~AtomRendererClient();

  void DidClearWindowObject(content::RenderFrame* render_frame);
  void DidCreateScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame);
  void WillReleaseScriptContext(
      v8::Handle<v8::Context> context, content::RenderFrame* render_frame);

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
  blink::WebSpeechSynthesizer* OverrideSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            blink::WebLocalFrame* frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  bool ShouldFork(blink::WebLocalFrame* frame,
                  const GURL& url,
                  const std::string& http_method,
                  bool is_initial_navigation,
                  bool is_server_redirect,
                  bool* send_referrer) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const std::string& mime_type,
      const GURL& original_url) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;

  std::unique_ptr<NodeBindings> node_bindings_;
  std::unique_ptr<AtomBindings> atom_bindings_;
  std::unique_ptr<PreferencesManager> preferences_manager_;

  DISALLOW_COPY_AND_ASSIGN(AtomRendererClient);
};

}  // namespace atom
