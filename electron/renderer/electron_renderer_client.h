// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_RENDERER_ELECTRON_RENDERER_CLIENT_H_
#define ELECTRON_RENDERER_ELECTRON_RENDERER_CLIENT_H_

#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/render_process_observer.h"

namespace electron {

class ElectronBindings;
class NodeBindings;

class ElectronRendererClient : public content::ContentRendererClient,
                           public content::RenderProcessObserver {
 public:
  ElectronRendererClient();
  virtual ~ElectronRendererClient();

  void DidCreateScriptContext(v8::Handle<v8::Context> context);
  void WillReleaseScriptContext(v8::Handle<v8::Context> context);

 private:
  enum NodeIntegration {
    ALL,
    EXCEPT_IFRAME,
    MANUAL_ENABLE_IFRAME,
    DISABLE,
  };

  // content::RenderProcessObserver:
  void WebKitInitialized() override;

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
  void AddKeySystems(std::vector<media::KeySystemInfo>* key_systems) override;

  scoped_ptr<NodeBindings> node_bindings_;
  scoped_ptr<ElectronBindings> electron_bindings_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRendererClient);
};

}  // namespace electron

#endif  // ELECTRON_RENDERER_ELECTRON_RENDERER_CLIENT_H_
