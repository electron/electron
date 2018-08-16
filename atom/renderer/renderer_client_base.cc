// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/renderer_client_base.h"

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
#include "base/process/process_handle.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "chrome/renderer/media/chrome_key_systems.h"
#include "chrome/renderer/printing/print_web_view_helper.h"
#include "chrome/renderer/tts_dispatcher.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h"
#include "third_party/WebKit/public/web/WebCustomElement.h"  // NOLINT(build/include_alpha)
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"

#if defined(OS_MACOSX)
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

#if defined(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#endif  // defined(ENABLE_PDF_VIEWER)

#if defined(ENABLE_PEPPER_FLASH)
#include "chrome/renderer/pepper/pepper_helper.h"
#endif  // defined(ENABLE_PEPPER_FLASH)

// This is defined in later versions of Chromium, remove this if you see
// compiler complaining duplicate defines.
#if defined(OS_WIN) || defined(OS_FUCHSIA)
#define CrPRIdPid "ld"
#else
#define CrPRIdPid "d"
#endif

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

std::vector<std::string> ParseSchemesCLISwitch(const char* switch_name) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string custom_schemes = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(custom_schemes, ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

RendererClientBase::RendererClientBase() {
  // Parse --standard-schemes=scheme1,scheme2
  std::vector<std::string> standard_schemes_list =
      ParseSchemesCLISwitch(switches::kStandardSchemes);
  for (const std::string& scheme : standard_schemes_list)
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITHOUT_PORT);
  isolated_world_ = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kContextIsolation);
}

RendererClientBase::~RendererClientBase() {}

void RendererClientBase::DidCreateScriptContext(
    v8::Handle<v8::Context> context,
    content::RenderFrame* render_frame) {
  // global.setHidden("contextId", `${processId}-${++next_context_id_}`)
  std::string context_id = base::StringPrintf(
      "%" CrPRIdPid "-%d", base::GetCurrentProcId(), ++next_context_id_);
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::String> key = mate::StringToSymbol(isolate, "contextId");
  v8::Local<v8::Private> private_key = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Value> value = mate::ConvertToV8(isolate, context_id);
  context->Global()->SetPrivate(context, private_key, value);
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
  blink::SchemeRegistry::RegisterURLSchemeAsCORSEnabled(extension_scheme);
  blink::SchemeRegistry::RegisterURLSchemeAsBypassingContentSecurityPolicy(
      extension_scheme);

  // Parse --secure-schemes=scheme1,scheme2
  std::vector<std::string> secure_schemes_list =
      ParseSchemesCLISwitch(switches::kSecureSchemes);
  for (const std::string& scheme : secure_schemes_list)
    blink::SchemeRegistry::RegisterURLSchemeAsSecure(
        WTF::String::FromUTF8(scheme.data(), scheme.length()));

  // Allow file scheme to handle service worker by default.
  // FIXME(zcbenz): Can this be moved elsewhere?
  blink::WebSecurityPolicy::RegisterURLSchemeAsAllowingServiceWorkers("file");
  blink::SchemeRegistry::RegisterURLSchemeAsSupportingFetchAPI("file");

  preferences_manager_.reset(new PreferencesManager);

#if defined(OS_WIN)
  // Set ApplicationUserModelID in renderer process.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
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
#if defined(ENABLE_PEPPER_FLASH)
  new PepperHelper(render_frame);
#endif
  new ContentSettingsObserver(render_frame);
  new printing::PrintWebViewHelper(render_frame);

  // This is required for widevine plugin detection provided during runtime.
  blink::ResetPluginCache();

#if defined(ENABLE_PDF_VIEWER)
  // Allow access to file scheme from pdf viewer.
  blink::WebSecurityPolicy::AddOriginAccessWhitelistEntry(
      GURL(kPdfViewerUIOrigin), "file", "", true);
#endif  // defined(ENABLE_PDF_VIEWER)
}

void RendererClientBase::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view);
  blink::WebFrameWidget* web_frame_widget = render_view->GetWebFrameWidget();
  if (!web_frame_widget)
    return;

  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(switches::kGuestInstanceID)) {  // webview.
    web_frame_widget->SetBaseBackgroundColor(SK_ColorTRANSPARENT);
  } else {  // normal window.
    std::string name = cmd->GetSwitchValueASCII(switches::kBackgroundColor);
    SkColor color = name.empty() ? SK_ColorTRANSPARENT : ParseHexColor(name);
    web_frame_widget->SetBaseBackgroundColor(color);
  }
}

void RendererClientBase::DidClearWindowObject(
    content::RenderFrame* render_frame) {
  // Make sure every page will get a script context created.
  render_frame->GetWebFrame()->ExecuteScript(blink::WebScriptSource("void 0"));
}

std::unique_ptr<blink::WebSpeechSynthesizer>
RendererClientBase::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return std::make_unique<TtsDispatcher>(client);
}

bool RendererClientBase::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mime_type.Utf8() == content::kBrowserPluginMimeType ||
#if defined(ENABLE_PDF_VIEWER)
      params.mime_type.Utf8() == kPdfPluginMimeType ||
#endif  // defined(ENABLE_PDF_VIEWER)
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

void RendererClientBase::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
  AddChromeKeySystems(key_systems);
}

v8::Local<v8::Context> RendererClientBase::GetContext(
    blink::WebLocalFrame* frame,
    v8::Isolate* isolate) const {
  if (isolated_world())
    return frame->WorldScriptContext(isolate, World::ISOLATED_WORLD);
  else
    return frame->MainWorldScriptContext();
}

}  // namespace atom
