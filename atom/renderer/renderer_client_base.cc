// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/renderer_client_base.h"

#include <memory>
#include <string>
#include <vector>

#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_autofill_agent.h"
#include "atom/renderer/atom_render_frame_observer.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "atom/renderer/content_settings_observer.h"
#include "atom/renderer/preferences_manager.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "electron/buildflags/buildflags.h"
#include "native_mate/dictionary.h"
#include "printing/buildflags/buildflags.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_custom_element.h"  // NOLINT(build/include_alpha)
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_plugin_params.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "third_party/blink/public/web/web_security_policy.h"
#include "third_party/blink/public/web/web_view.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"

#if defined(OS_MACOSX)
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#if BUILDFLAG(ENABLE_PEPPER_FLASH)
#include "chrome/renderer/pepper/pepper_helper.h"
#endif  // BUILDFLAG(ENABLE_PEPPER_FLASH)

#if BUILDFLAG(ENABLE_TTS)
#include "chrome/renderer/tts_dispatcher.h"
#endif  // BUILDFLAG(ENABLE_TTS)

#if BUILDFLAG(ENABLE_PRINTING)
#include "atom/renderer/printing/print_render_frame_helper_delegate.h"
#include "components/printing/renderer/print_render_frame_helper.h"
#include "printing/print_settings.h"
#endif  // BUILDFLAG(ENABLE_PRINTING)

namespace atom {

namespace {

v8::Local<v8::Value> GetRenderProcessPreferences(
    const PreferencesManager* preferences_manager,
    v8::Isolate* isolate) {
  if (preferences_manager->preferences())
    return mate::ConvertToV8(isolate, *preferences_manager->preferences());
  else
    return v8::Null(isolate);
}

std::vector<std::string> ParseSchemesCLISwitch(base::CommandLine* command_line,
                                               const char* switch_name) {
  std::string custom_schemes = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(custom_schemes, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

void SetHiddenValue(v8::Handle<v8::Context> context,
                    const base::StringPiece& key,
                    v8::Local<v8::Value> value) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Private> privateKey =
      v8::Private::ForApi(isolate, mate::StringToV8(isolate, key));
  context->Global()->SetPrivate(context, privateKey, value);
}

}  // namespace

RendererClientBase::RendererClientBase() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  // Parse --standard-schemes=scheme1,scheme2
  std::vector<std::string> standard_schemes_list =
      ParseSchemesCLISwitch(command_line, switches::kStandardSchemes);
  for (const std::string& scheme : standard_schemes_list)
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITH_HOST);
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

RendererClientBase::~RendererClientBase() {}

void RendererClientBase::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  // global.setHidden("contextId", `${processHostId}-${++next_context_id_}`)
  auto context_id = base::StringPrintf(
      "%s-%" PRId64, renderer_client_id_.c_str(), ++next_context_id_);
  v8::Isolate* isolate = context->GetIsolate();
  SetHiddenValue(context, "contextId", mate::ConvertToV8(isolate, context_id));

  auto* command_line = base::CommandLine::ForCurrentProcess();
  bool enableRemoteModule =
      !command_line->HasSwitch(switches::kDisableRemoteModule);
  SetHiddenValue(context, "enableRemoteModule",
                 mate::ConvertToV8(isolate, enableRemoteModule));
}

void RendererClientBase::AddRenderBindings(
    v8::Isolate* isolate,
    v8::Local<v8::Object> binding_object) {
  mate::Dictionary dict(isolate, binding_object);
  dict.SetMethod(
      "getRenderProcessPreferences",
      base::Bind(GetRenderProcessPreferences, preferences_manager_.get()));
}

void RendererClientBase::RenderThreadStarted() {
  auto* command_line = base::CommandLine::ForCurrentProcess();

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

  preferences_manager_.reset(new PreferencesManager);

#if defined(OS_WIN)
  // Set ApplicationUserModelID in renderer process.
  base::string16 app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void RendererClientBase::RenderFrameCreated(
    content::RenderFrame* render_frame) {
#if defined(TOOLKIT_VIEWS)
  new AutofillAgent(render_frame);
#endif
#if BUILDFLAG(ENABLE_PEPPER_FLASH)
  new PepperHelper(render_frame);
#endif
  new ContentSettingsObserver(render_frame);
#if BUILDFLAG(ENABLE_PRINTING)
  new printing::PrintRenderFrameHelper(
      render_frame, std::make_unique<atom::PrintRenderFrameHelperDelegate>());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  // Allow access to file scheme from pdf viewer.
  blink::WebSecurityPolicy::AddOriginAccessWhitelistEntry(
      GURL(kPdfViewerUIOrigin), "file", "", true);
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

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
}

void RendererClientBase::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view);
}

void RendererClientBase::DidClearWindowObject(
    content::RenderFrame* render_frame) {
  // Make sure every page will get a script context created.
  render_frame->GetWebFrame()->ExecuteScript(blink::WebScriptSource("void 0"));
}

std::unique_ptr<blink::WebSpeechSynthesizer>
RendererClientBase::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
#if BUILDFLAG(ENABLE_TTS)
  return std::make_unique<TtsDispatcher>(client);
#else
  return nullptr;
#endif
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

v8::Local<v8::Context> RendererClientBase::GetContext(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate) const {
  if (isolated_world())
    return frame->WorldScriptContext(isolate, World::ISOLATED_WORLD);
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

bool RendererClientBase::IsWebViewFrame(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) const {
  auto* isolate = context->GetIsolate();

  if (render_frame->IsMainFrame())
    return false;

  mate::Dictionary window_dict(
      isolate, GetContext(render_frame->GetWebFrame(), isolate)->Global());

  v8::Local<v8::Object> frame_element;
  if (!window_dict.Get("frameElement", &frame_element))
    return false;

  mate::Dictionary frame_element_dict(isolate, frame_element);

  v8::Local<v8::Object> internal;
  if (!frame_element_dict.GetHidden("internal", &internal))
    return false;

  return !internal.IsEmpty();
}

}  // namespace atom
