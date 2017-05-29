// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/renderer_client_base.h"

#include <string>
#include <vector>

#include "atom/common/atom_constants.h"
#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_autofill_agent.h"
#include "atom/renderer/atom_render_frame_observer.h"
#include "atom/renderer/content_settings_observer.h"
#include "atom/renderer/guest_view_container.h"
#include "atom/renderer/preferences_manager.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "chrome/renderer/media/chrome_key_systems.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "chrome/renderer/printing/print_web_view_helper.h"
#include "chrome/renderer/tts_dispatcher.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_view.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebCustomElement.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "third_party/WebKit/Source/platform/weborigin/SchemeRegistry.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

namespace atom {

namespace {

v8::Local<v8::Value> GetRenderProcessPreferences(
    const PreferencesManager* preferences_manager, v8::Isolate* isolate) {
  if (preferences_manager->preferences())
    return mate::ConvertToV8(isolate, *preferences_manager->preferences());
  else
    return v8::Null(isolate);
}

std::vector<std::string> ParseSchemesCLISwitch(const char* switch_name) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string custom_schemes = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(
      custom_schemes, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

RendererClientBase::RendererClientBase() {
  // Parse --standard-schemes=scheme1,scheme2
  std::vector<std::string> standard_schemes_list =
      ParseSchemesCLISwitch(switches::kStandardSchemes);
  for (const std::string& scheme : standard_schemes_list)
    url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITHOUT_PORT);
}

RendererClientBase::~RendererClientBase() {
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
  blink::WebCustomElement::addEmbedderCustomElementName("webview");
  blink::WebCustomElement::addEmbedderCustomElementName("browserplugin");

  // Parse --secure-schemes=scheme1,scheme2
  std::vector<std::string> secure_schemes_list =
      ParseSchemesCLISwitch(switches::kSecureSchemes);
  for (const std::string& scheme : secure_schemes_list)
    blink::SchemeRegistry::registerURLSchemeAsSecure(
        WTF::String::fromUTF8(scheme.data(), scheme.length()));

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

#if defined(OS_MACOSX)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  bool scroll_bounce = command_line->HasSwitch(switches::kScrollBounce);
  base::ScopedCFTypeRef<CFStringRef> rubber_banding_key(
    base::SysUTF8ToCFStringRef("NSScrollViewRubberbanding"));
  CFPreferencesSetAppValue(rubber_banding_key,
                           scroll_bounce ? kCFBooleanTrue : kCFBooleanFalse,
                           kCFPreferencesCurrentApplication);
  CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
#endif
}

void RendererClientBase::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new AtomRenderFrameObserver(render_frame, this);
  new AutofillAgent(render_frame);
  new PepperHelper(render_frame);
  new ContentSettingsObserver(render_frame);
  new printing::PrintWebViewHelper(render_frame);

  // Allow file scheme to handle service worker by default.
  // FIXME(zcbenz): Can this be moved elsewhere?
  blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers("file");

  // This is required for widevine plugin detection provided during runtime.
  blink::resetPluginCache();

  // Allow access to file scheme from pdf viewer.
  blink::WebSecurityPolicy::addOriginAccessWhitelistEntry(
      GURL(kPdfViewerUIOrigin), "file", "", true);
}

void RendererClientBase::RenderViewCreated(content::RenderView* render_view) {
  blink::WebFrameWidget* web_frame_widget = render_view->GetWebFrameWidget();
  if (!web_frame_widget)
    return;

  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  if (cmd->HasSwitch(switches::kGuestInstanceID)) {  // webview.
    web_frame_widget->setBaseBackgroundColor(SK_ColorTRANSPARENT);
  } else {  // normal window.
    // If backgroundColor is specified then use it.
    std::string name = cmd->GetSwitchValueASCII(switches::kBackgroundColor);
    // Otherwise use white background.
    SkColor color = name.empty() ? SK_ColorWHITE : ParseHexColor(name);
    web_frame_widget->setBaseBackgroundColor(color);
  }
}

void RendererClientBase::DidClearWindowObject(
    content::RenderFrame* render_frame) {
  // Make sure every page will get a script context created.
  render_frame->GetWebFrame()->executeScript(blink::WebScriptSource("void 0"));
}

blink::WebSpeechSynthesizer* RendererClientBase::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return new TtsDispatcher(client);
}

bool RendererClientBase::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (params.mimeType.utf8() == content::kBrowserPluginMimeType ||
      params.mimeType.utf8() == kPdfPluginMimeType ||
      command_line->HasSwitch(switches::kEnablePlugins))
    return false;

  *plugin = nullptr;
  return true;
}

content::BrowserPluginDelegate* RendererClientBase::CreateBrowserPluginDelegate(
    content::RenderFrame* render_frame,
    const std::string& mime_type,
    const GURL& original_url) {
  if (mime_type == content::kBrowserPluginMimeType) {
    return new GuestViewContainer(render_frame);
  } else {
    return nullptr;
  }
}

void RendererClientBase::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
  AddChromeKeySystems(key_systems);
}

}  // namespace atom
