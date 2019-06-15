// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
#define SHELL_RENDERER_RENDERER_CLIENT_BASE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "shell/common/api/api.mojom.h"
#include "third_party/blink/public/web/web_local_frame.h"
// In SHARED_INTERMEDIATE_DIR.
#include "widevine_cdm_version.h"  // NOLINT(build/include)

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "chrome/renderer/media/chrome_key_systems_provider.h"  // nogncheck
#endif

namespace electron {

class RendererClientBase : public content::ContentRendererClient {
 public:
  RendererClientBase();
  ~RendererClientBase() override;

  virtual void DidCreateScriptContext(v8::Handle<v8::Context> context,
                                      content::RenderFrame* render_frame);
  virtual void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                        content::RenderFrame* render_frame) = 0;
  virtual void DidClearWindowObject(content::RenderFrame* render_frame);
  virtual void SetupMainWorldOverrides(v8::Handle<v8::Context> context,
                                       content::RenderFrame* render_frame) = 0;
  virtual void SetupExtensionWorldOverrides(v8::Handle<v8::Context> context,
                                            content::RenderFrame* render_frame,
                                            int world_id) = 0;

  const mojom::WebPreferences& GetWebPreferences(
      content::RenderFrame* render_frame) const;

  // Get the context that the Electron API is running in.
  v8::Local<v8::Context> GetContext(blink::WebLocalFrame* frame,
                                    v8::Isolate* isolate) const;
  // Executes a given v8 Script
  static v8::Local<v8::Value> RunScript(v8::Local<v8::Context> context,
                                        v8::Local<v8::String> source);

 protected:
  void AddRenderBindings(v8::Isolate* isolate,
                         v8::Local<v8::Object> binding_object);

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame*) override;
  std::unique_ptr<blink::WebSpeechSynthesizer> OverrideSpeechSynthesizer(
      blink::WebSpeechSynthesizerClient* client) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;
  bool IsKeySystemsUpdateNeeded() override;
  void DidSetUserAgent(const std::string& user_agent) override;

 private:
  v8::Local<v8::Value> GetWebPreferences(content::RenderFrame* render_frame,
                                         v8::Isolate* isolate) const;

#if defined(WIDEVINE_CDM_AVAILABLE)
  ChromeKeySystemsProvider key_systems_provider_;
#endif
  mutable std::unordered_map<content::RenderFrame*, mojom::WebPreferences>
      web_preferences_;
  std::string renderer_client_id_;
  // An increasing ID used for indentifying an V8 context in this process.
  int64_t next_context_id_ = 0;
};

}  // namespace electron

#endif  // SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
