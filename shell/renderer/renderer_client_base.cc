// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/renderer_client_base.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "components/network_hints/renderer/web_prescient_networking_impl.h"
#include "content/common/buildflags.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "electron/buildflags/buildflags.h"
#include "media/blink/multibuffer_data_source.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/options_switches.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/browser_exposed_renderer_interfaces.h"
#include "shell/renderer/content_settings_observer.h"
#include "shell/renderer/electron_api_service_impl.h"
#include "shell/renderer/electron_autofill_agent.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_custom_element.h"  // NOLINT(build/include_alpha)
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"  // nogncheck

#if defined(OS_MACOSX)
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/renderer/spellcheck_provider.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/renderer/pepper/chrome_pdf_print_client.h"
#include "shell/common/electron_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#if BUILDFLAG(ENABLE_PLUGINS)
#include "shell/renderer/pepper_helper.h"
#endif  // BUILDFLAG(ENABLE_PLUGINS)

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/renderer/print_render_frame_helper.h"
#include "printing/print_settings.h"  // nogncheck
#include "shell/renderer/printing/print_render_frame_helper_delegate.h"
#endif  // BUILDFLAG(ENABLE_PRINTING)

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/webplugininfo.h"
#include "extensions/common/constants.h"
#include "extensions/common/extensions_client.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container_dispatcher.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container.h"
#include "extensions/renderer/guest_view/mime_handler_view/mime_handler_view_container_manager.h"
#include "shell/common/extensions/electron_extensions_client.h"
#include "shell/renderer/extensions/electron_extensions_renderer_client.h"
#endif  // BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

namespace electron {

namespace {

std::vector<std::string> ParseSchemesCLISwitch(base::CommandLine* command_line,
                                               const char* switch_name) {
  std::string custom_schemes = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(custom_schemes, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

RendererClientBase::RendererClientBase() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  // Parse --standard-schemes=scheme1,scheme2
  std::vector<std::string> standard_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kStandardSchemes);
  for (const std::string& scheme : standard_schemes_list)
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITH_HOST);
  // Parse --cors-schemes=scheme1,scheme2
  std::vector<std::string> cors_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kCORSSchemes);
  for (const std::string& scheme : cors_schemes_list)
    url::AddCorsEnabledScheme(scheme.c_str());
  // Parse --streaming-schemes=scheme1,scheme2
  std::vector<std::string> streaming_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kStreamingSchemes);
  for (const std::string& scheme : streaming_schemes_list)
    media::AddStreamingScheme(scheme.c_str());
  isolated_world_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kContextIsolation);
  // We rely on the unique process host id which is notified to the
  // renderer process via command line switch from the content layer,
  // if this switch is removed from the content layer for some reason,
  // we should define our own.
  DCHECK(command_line->HasSwitch(::switches::kRendererClientId));
  renderer_client_id_ =
      command_line->GetSwitchValueASCII(::switches::kRendererClientId);
}

RendererClientBase::~RendererClientBase() = default;

void RendererClientBase::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  // global.setHidden("contextId", `${processHostId}-${++next_context_id_}`)
  auto context_id = base::StringPrintf(
      "%s-%" PRId64, renderer_client_id_.c_str(), ++next_context_id_);
  gin_helper::Dictionary global(context->GetIsolate(), context->Global());
  global.SetHidden("contextId", context_id);

#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  auto* command_line = base::CommandLine::ForCurrentProcess();
  bool enableRemoteModule =
      command_line->HasSwitch(switches::kEnableRemoteModule);
  global.SetHidden("enableRemoteModule", enableRemoteModule);
#endif
}

void RendererClientBase::AddRenderBindings(
    v8::Isolate* isolate,
    v8::Local<v8::Object> binding_object) {}

void RendererClientBase::RenderThreadStarted() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

#if BUILDFLAG(USE_EXTERNAL_POPUP_MENU)
  // On macOS, popup menus are rendered by the main process by default.
  // This causes problems in OSR, since when the popup is rendered separately,
  // it won't be captured in the rendered image.
  if (command_line->HasSwitch(options::kOffscreen)) {
    blink::WebView::SetUseExternalPopupMenus(false);
  }
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* thread = content::RenderThread::Get();

  extensions_client_.reset(CreateExtensionsClient());
  extensions::ExtensionsClient::Set(extensions_client_.get());

  extensions_renderer_client_.reset(new ElectronExtensionsRendererClient);
  extensions::ExtensionsRendererClient::Set(extensions_renderer_client_.get());

  thread->AddObserver(extensions_renderer_client_->GetDispatcher());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // Enables printing from Chrome PDF viewer.
  pdf::PepperPDFHost::SetPrintClient(new ChromePDFPrintClient());
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  if (command_line->HasSwitch(switches::kEnableSpellcheck))
    spellcheck_ = std::make_unique<SpellCheck>(this);
#endif

  blink::WebCustomElement::AddEmbedderCustomElementName("webview");
  blink::WebCustomElement::AddEmbedderCustomElementName("browserplugin");

  WTF::String extension_scheme("chrome-extension");
  // Extension resources are HTTP-like and safe to expose to the fetch API. The
  // rules for the fetch API are consistent with XHR.
  blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI(
      extension_scheme);
  // Extension resources, when loaded as the top-level document, should bypass
  // Blink's strict first-party origin checks.
  blink::SchemeRegistry::RegisterURLSchemeAsFirstPartyWhenTopLevel(
      extension_scheme);
  // In Chrome we should set extension's origins to match the pages they can
  // work on, but in Electron currently we just let extensions do anything.
  blink::SchemeRegistry::RegisterURLSchemeAsSecure(extension_scheme);
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      extension_scheme);

  // Parse --secure-schemes=scheme1,scheme2
  std::vector<std::string> secure_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kSecureSchemes);
  for (const std::string& scheme : secure_schemes_list)
    blink::SchemeRegistry::RegisterURLSchemeAsSecure(
        WTF::String::FromUTF8(scheme.data(), scheme.length()));

  std::vector<std::string> fetch_enabled_schemes =
      ParseSchemesCLISwitch(command_line, switches::kFetchSchemes);
  for (const std::string& scheme : fetch_enabled_schemes) {
    blink::WebSecurityPolicy::RegisterURLSchemeAsSupportingFetchAPI(
        blink::WebString::FromASCII(scheme));
  }

  std::vector<std::string> service_worker_schemes =
      ParseSchemesCLISwitch(command_line, switches::kServiceWorkerSchemes);
  for (const std::string& scheme : service_worker_schemes)
    blink::WebSecurityPolicy::RegisterURLSchemeAsAllowingServiceWorkers(
        blink::WebString::FromASCII(scheme));

  std::vector<std::string> csp_bypassing_schemes =
      ParseSchemesCLISwitch(command_line, switches::kBypassCSPSchemes);
  for (const std::string& scheme : csp_bypassing_schemes)
    blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
        WTF::String::FromUTF8(scheme.data(), scheme.length()));

  // Allow file scheme to handle service worker by default.
  // FIXME(zcbenz): Can this be moved elsewhere?
  blink::WebSecurityPolicy::RegisterURLSchemeAsAllowingServiceWorkers("file");
  blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI("file");

#if defined(OS_WIN)
  // Set ApplicationUserModelID in renderer process.
  base::string16 app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void RendererClientBase::ExposeInterfacesToBrowser(mojo::BinderMap* binders) {
  // NOTE: Do not add binders directly within this method. Instead, modify the
  // definition of |ExposeElectronRendererInterfacesToBrowser()| to ensure
  // security review coverage.
  ExposeElectronRendererInterfacesToBrowser(this, binders);
}

void RendererClientBase::RenderFrameCreated(
    content::RenderFrame* render_frame) {
#if defined(TOOLKIT_VIEWS)
  new AutofillAgent(render_frame,
                    render_frame->GetAssociatedInterfaceRegistry());
#endif
#if BUILDFLAG(ENABLE_PLUGINS)
  new PepperHelper(render_frame);
#endif
  new ContentSettingsObserver(render_frame);
#if BUILDFLAG(ENABLE_PRINTING)
  new printing::PrintRenderFrameHelper(
      render_frame,
      std::make_unique<electron::PrintRenderFrameHelperDelegate>());
#endif

  // Note: ElectronApiServiceImpl has to be created now to capture the
  // DidCreateDocumentElement event.
  auto* service = new ElectronApiServiceImpl(render_frame, this);
  render_frame->GetAssociatedInterfaceRegistry()->AddInterface(
      base::BindRepeating(&ElectronApiServiceImpl::BindTo,
                          service->GetWeakPtr()));

  content::RenderView* render_view = render_frame->GetRenderView();
  if (render_frame->IsMainFrame() && render_view) {
    blink::WebView* webview = render_view->GetWebView();
    if (webview) {
      base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
      if (cmd->HasSwitch(switches::kGuestInstanceID)) {  // webview.
        webview->SetBaseBackgroundColor(SK_ColorTRANSPARENT);
      } else {  // normal window.
        std::string name = cmd->GetSwitchValueASCII(switches::kBackgroundColor);
        SkColor color =
            name.empty() ? SK_ColorTRANSPARENT : ParseHexColor(name);
        webview->SetBaseBackgroundColor(color);
      }
    }
  }

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  auto* dispatcher = extensions_renderer_client_->GetDispatcher();
  // ExtensionFrameHelper destroys itself when the RenderFrame is destroyed.
  new extensions::ExtensionFrameHelper(render_frame, dispatcher);

  dispatcher->OnRenderFrameCreated(render_frame);

  render_frame->GetAssociatedInterfaceRegistry()->AddInterface(
      base::BindRepeating(
          &extensions::MimeHandlerViewContainerManager::BindReceiver,
          render_frame->GetRoutingID()));
#endif

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnableSpellcheck))
    new SpellCheckProvider(render_frame, spellcheck_.get(), this);
#endif
}

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
void RendererClientBase::GetInterface(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  // TODO(crbug.com/977637): Get rid of the use of this implementation of
  // |service_manager::LocalInterfaceProvider|. This was done only to avoid
  // churning spellcheck code while eliminating the "chrome" and
  // "chrome_renderer" services. Spellcheck is (and should remain) the only
  // consumer of this implementation.
  content::RenderThread::Get()->BindHostReceiver(
      mojo::GenericPendingReceiver(interface_name, std::move(interface_pipe)));
}
#endif

void RendererClientBase::DidClearWindowObject(
    content::RenderFrame* render_frame) {
  // Make sure every page will get a script context created.
  render_frame->GetWebFrame()->ExecuteScript(blink::WebScriptSource("void 0"));
}

bool RendererClientBase::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mime_type.Utf8() == content::kBrowserPluginMimeType ||
#if BUILDFLAG(ENABLE_PDF_VIEWER)
      params.mime_type.Utf8() == kPdfPluginMimeType ||
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

void RendererClientBase::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
#if defined(WIDEVINE_CDM_AVAILABLE)
  key_systems_provider_.AddSupportedKeySystems(key_systems);
#endif
}

bool RendererClientBase::IsKeySystemsUpdateNeeded() {
#if defined(WIDEVINE_CDM_AVAILABLE)
  return key_systems_provider_.IsKeySystemsUpdateNeeded();
#else
  return false;
#endif
}

void RendererClientBase::DidSetUserAgent(const std::string& user_agent) {
#if BUILDFLAG(ENABLE_PRINTING)
  printing::SetAgent(user_agent);
#endif
}

content::BrowserPluginDelegate* RendererClientBase::CreateBrowserPluginDelegate(
    content::RenderFrame* render_frame,
    const content::WebPluginInfo& info,
    const std::string& mime_type,
    const GURL& original_url) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // TODO(nornagon): check the mime type isn't content::kBrowserPluginMimeType?
  return new extensions::MimeHandlerViewContainer(render_frame, info, mime_type,
                                                  original_url);
#else
  return nullptr;
#endif
}

bool RendererClientBase::IsPluginHandledExternally(
    content::RenderFrame* render_frame,
    const blink::WebElement& plugin_element,
    const GURL& original_url,
    const std::string& mime_type) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  DCHECK(plugin_element.HasHTMLTagName("object") ||
         plugin_element.HasHTMLTagName("embed"));
  // TODO(nornagon): this info should be shared with the data in
  // electron_content_client.cc / ComputeBuiltInPlugins.
  content::WebPluginInfo info;
  info.type = content::WebPluginInfo::PLUGIN_TYPE_BROWSER_PLUGIN;
  info.name = base::UTF8ToUTF16("Chromium PDF Viewer");
  info.path = base::FilePath::FromUTF8Unsafe(extension_misc::kPdfExtensionId);
  info.background_color = content::WebPluginInfo::kDefaultBackgroundColor;
  info.mime_types.emplace_back("application/pdf", "pdf",
                               "Portable Document Format");
  return extensions::MimeHandlerViewContainerManager::Get(
             content::RenderFrame::FromWebFrame(
                 plugin_element.GetDocument().GetFrame()),
             true /* create_if_does_not_exist */)
      ->CreateFrameContainer(plugin_element, original_url, mime_type, info);
#else
  return false;
#endif
}

bool RendererClientBase::IsOriginIsolatedPepperPlugin(
    const base::FilePath& plugin_path) {
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  return plugin_path.value() == kPdfPluginPath;
#else
  return false;
#endif
}

std::unique_ptr<blink::WebPrescientNetworking>
RendererClientBase::CreatePrescientNetworking(
    content::RenderFrame* render_frame) {
  return std::make_unique<network_hints::WebPrescientNetworkingImpl>(
      render_frame);
}

void RendererClientBase::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentStart(render_frame);
#endif
}

void RendererClientBase::RunScriptsAtDocumentIdle(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentIdle(render_frame);
#endif
}

void RendererClientBase::RunScriptsAtDocumentEnd(
    content::RenderFrame* render_frame) {
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions_renderer_client_.get()->RunScriptsAtDocumentEnd(render_frame);
#endif
}

v8::Local<v8::Context> RendererClientBase::GetContext(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate) const {
  if (isolated_world())
    return frame->WorldScriptContext(isolate, WorldIDs::ISOLATED_WORLD_ID);
  else
    return frame->MainWorldScriptContext();
}

v8::Local<v8::Value> RendererClientBase::RunScript(
    v8::Local<v8::Context> context,
    v8::Local<v8::String> source) {
  auto maybe_script = v8::Script::Compile(context, source);
  v8::Local<v8::Script> script;
  if (!maybe_script.ToLocal(&script))
    return v8::Local<v8::Value>();
  return script->Run(context).ToLocalChecked();
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
extensions::ExtensionsClient* RendererClientBase::CreateExtensionsClient() {
  return new ElectronExtensionsClient;
}
#endif

bool RendererClientBase::IsWebViewFrame(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) const {
  auto* isolate = context->GetIsolate();

  if (render_frame->IsMainFrame())
    return false;

  gin::Dictionary window_dict(
      isolate, GetContext(render_frame->GetWebFrame(), isolate)->Global());

  v8::Local<v8::Object> frame_element;
  if (!window_dict.Get("frameElement", &frame_element))
    return false;

  gin_helper::Dictionary frame_element_dict(isolate, frame_element);

  v8::Local<v8::Object> internal;
  if (!frame_element_dict.GetHidden("internal", &internal))
    return false;

  return !internal.IsEmpty();
}

}  // namespace electron
