// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
#define SHELL_RENDERER_RENDERER_CLIENT_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "electron/buildflags/buildflags.h"
#include "third_party/blink/public/web/web_local_frame.h"
// In SHARED_INTERMEDIATE_DIR.
#include "widevine_cdm_version.h"  // NOLINT(build/include_directory)

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "chrome/renderer/media/chrome_key_systems_provider.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/local_interface_provider.h"

class SpellCheck;
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
namespace extensions {
class ExtensionsClient;
}
#endif

namespace electron {

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsRendererClient;
#endif

class RendererClientBase : public content::ContentRendererClient
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
    ,
                           public service_manager::LocalInterfaceProvider
#endif
{
 public:
  RendererClientBase();
  ~RendererClientBase() override;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  // service_manager::LocalInterfaceProvider implementation.
  void GetInterface(const std::string& name,
                    mojo::ScopedMessagePipeHandle request_handle) override;
#endif

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

  std::unique_ptr<blink::WebPrescientNetworking> CreatePrescientNetworking(
      content::RenderFrame* render_frame) override;
  bool isolated_world() const { return isolated_world_; }

  // Get the context that the Electron API is running in.
  v8::Local<v8::Context> GetContext(blink::WebLocalFrame* frame,
                                    v8::Isolate* isolate) const;
  // Executes a given v8 Script
  static v8::Local<v8::Value> RunScript(v8::Local<v8::Context> context,
                                        v8::Local<v8::String> source);

  // v8Util.getHiddenValue(window.frameElement, 'internal')
  bool IsWebViewFrame(v8::Handle<v8::Context> context,
                      content::RenderFrame* render_frame) const;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellCheck* GetSpellCheck() { return spellcheck_.get(); }
#endif

 protected:
  void AddRenderBindings(v8::Isolate* isolate,
                         v8::Local<v8::Object> binding_object);

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  void RenderFrameCreated(content::RenderFrame*) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;
  bool IsKeySystemsUpdateNeeded() override;
  void DidSetUserAgent(const std::string& user_agent) override;
  content::BrowserPluginDelegate* CreateBrowserPluginDelegate(
      content::RenderFrame* render_frame,
      const content::WebPluginInfo& info,
      const std::string& mime_type,
      const GURL& original_url) override;
  bool IsPluginHandledExternally(content::RenderFrame* render_frame,
                                 const blink::WebElement& plugin_element,
                                 const GURL& original_url,
                                 const std::string& mime_type) override;
  bool IsOriginIsolatedPepperPlugin(const base::FilePath& plugin_path) override;

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame) override;

 protected:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // app_shell embedders may need custom extensions client interfaces.
  // This class takes ownership of the returned object.
  virtual extensions::ExtensionsClient* CreateExtensionsClient();
#endif

 private:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsRendererClient> extensions_renderer_client_;
#endif

#if defined(WIDEVINE_CDM_AVAILABLE)
  ChromeKeySystemsProvider key_systems_provider_;
#endif
  bool isolated_world_;
  std::string renderer_client_id_;
  // An increasing ID used for indentifying an V8 context in this process.
  int64_t next_context_id_ = 0;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  std::unique_ptr<SpellCheck> spellcheck_;
#endif
};

}  // namespace electron

#endif  // SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
