// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/security_warnings.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/converter.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom-shared.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/bindings/core/v8/local_window_proxy.h"  // nogncheck
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"  // nogncheck
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/local_dom_window.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/local_frame.h"  // nogncheck
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"  // nogncheck
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"  // nogncheck
#include "third_party/blink/renderer/core/timing/performance_entry.h"  // nogncheck
#include "third_party/blink/renderer/core/timing/window_performance.h"  // nogncheck
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"  // nogncheck
#include "url/gurl.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-microtask-queue.h"

namespace electron {

namespace {

constexpr std::string_view kMoreInformation =
    "\nFor more information and help, consult\n"
    "https://electronjs.org/docs/tutorial/security.\nThis warning will not "
    "show up\nonce the app is packaged.";

// Whether this looks like the stock `electron` binary rather than a packaged
// app, or the warnings were forced on or off through the environment.
bool SecurityWarningsEnabledForProcess() {
  static const bool enabled = [] {
    auto env = base::Environment::Create();
    if (env->HasVar("ELECTRON_DISABLE_SECURITY_WARNINGS"))
      return false;
    if (env->HasVar("ELECTRON_ENABLE_SECURITY_WARNINGS"))
      return true;
    base::FilePath::StringType exe =
        base::CommandLine::ForCurrentProcess()->GetProgram().value();
#if BUILDFLAG(IS_MAC)
    return base::EndsWith(exe, "MacOS/Electron") ||
           exe.find("Electron.app/Contents/Frameworks/") != std::string::npos;
#elif BUILDFLAG(IS_WIN)
    return base::EndsWith(exe, L"\\electron.exe");
#else
    return base::EndsWith(exe, "/electron");
#endif
  }();
  return enabled;
}

bool HasTruthyGlobal(v8::Local<v8::Context> context, std::string_view name) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Value> value;
  return context->Global()
             ->Get(context, gin::StringToV8(isolate, name))
             .ToLocal(&value) &&
         value->BooleanValue(isolate);
}

class SecurityWarnings : public content::RenderFrameObserver {
 public:
  explicit SecurityWarnings(content::RenderFrame* render_frame)
      : content::RenderFrameObserver(render_frame) {}

 private:
  // content::RenderFrameObserver:
  void DidFinishLoad() override {
    blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
    v8::Isolate* isolate = frame->GetAgentGroupScheduler()->Isolate();
    v8::HandleScope handle_scope(isolate);
    // A page that has not run any script has no main world context yet, and
    // so cannot have opted in; don't create one on its behalf.
    v8::Local<v8::Context> context =
        static_cast<blink::WebLocalFrameImpl*>(frame)
            ->GetFrame()
            ->WindowProxyMaybeUninitialized(
                blink::DOMWrapperWorld::MainWorld(isolate))
            ->ContextIfInitialized();
    if (context.IsEmpty() && !SecurityWarningsEnabledForProcess())
      return;
    if (context.IsEmpty())
      context = frame->MainWorldScriptContext();
    v8::MicrotasksScope microtasks_scope(
        context, v8::MicrotasksScope::kDoNotRunMicrotasks);
    v8::Context::Scope context_scope(context);
    // The page can opt out (or in) as the old preload-side check allowed.
    if (HasTruthyGlobal(context, "ELECTRON_DISABLE_SECURITY_WARNINGS"))
      return;
    if (!SecurityWarningsEnabledForProcess() &&
        !HasTruthyGlobal(context, "ELECTRON_ENABLE_SECURITY_WARNINGS")) {
      return;
    }

    const blink::web_pref::WebPreferences& prefs =
        render_frame()->GetBlinkPreferences();
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    GURL url(frame->GetDocument().Url());

    WarnAboutNodeWithRemoteContent(prefs, url);
    WarnAboutDisabledWebSecurity(prefs);
    WarnAboutInsecureResources(context);
    WarnAboutInsecureContentAllowed(prefs);
    WarnAboutExperimentalFeatures(command_line);
    WarnAboutEnableBlinkFeatures(command_line);
    WarnAboutInsecureCSP(context);
    WarnAboutAllowedPopups();
  }

  void OnDestruct() override { delete this; }

  void Warn(std::string_view title, std::string_view body) {
    render_frame()->GetWebFrame()->AddMessageToConsole(blink::WebConsoleMessage(
        blink::mojom::ConsoleMessageLevel::kWarning,
        blink::WebString::FromUtf8(
            base::StrCat({"Electron Security Warning (", title, ") ", body,
                          kMoreInformation}))));
  }

  // #1 Only load secure content
  void WarnAboutInsecureResources(v8::Local<v8::Context> context) {
    blink::LocalDOMWindow* window = blink::ToLocalDOMWindow(context);
    if (!window)
      return;
    std::string resources;
    for (const auto& entry :
         blink::DOMWindowPerformance::performance(*window)->getEntriesByType(
             blink::AtomicString("resource"))) {
      GURL resource(entry->name().Utf8());
      if ((resource.SchemeIs("http") || resource.SchemeIs("ftp")) &&
          !IsLocalhost(resource)) {
        base::StrAppend(&resources, {"\n- ", resource.spec()});
      }
    }
    if (resources.empty())
      return;
    Warn("Insecure Resources",
         base::StrCat({"This renderer process loads resources using insecure\n"
                       "  protocols. This exposes users of this app to "
                       "unnecessary security risks.\n  Consider loading the "
                       "following resources over HTTPS or FTPS. ",
                       resources, "\n  \n"}));
  }

  // #2 Disable the Node.js integration in all renderers that display remote
  // content
  void WarnAboutNodeWithRemoteContent(
      const blink::web_pref::WebPreferences& prefs,
      const GURL& url) {
    if (!prefs.node_integration || url.host() == "localhost")
      return;
    if (!url.SchemeIsHTTPOrHTTPS() && !url.SchemeIs("ftp") &&
        !url.SchemeIs("ftps")) {
      return;
    }
    Warn("Node.js Integration with Remote Content",
         base::StrCat({"This renderer process has Node.js integration enabled\n"
                       "    and attempted to load remote content from '",
                       url.spec(),
                       "'. This\n    exposes users of this app to severe "
                       "security risks.\n"}));
  }

  // #5 Do not disable websecurity
  void WarnAboutDisabledWebSecurity(
      const blink::web_pref::WebPreferences& prefs) {
    if (prefs.web_security_enabled)
      return;
    Warn("Disabled webSecurity",
         "This renderer process has \"webSecurity\" disabled. This\n  exposes "
         "users of this app to severe security risks.\n");
  }

  // #6 Define a Content-Security-Policy and use restrictive rules
  void WarnAboutInsecureCSP(v8::Local<v8::Context> context) {
    blink::ExecutionContext* execution_context =
        blink::ExecutionContext::From(context);
    if (!execution_context ||
        execution_context->GetContentSecurityPolicy()->ShouldCheckEval()) {
      return;
    }
    Warn("Insecure Content-Security-Policy",
         "This renderer process has either no Content Security\n  Policy set "
         "or a policy with \"unsafe-eval\" enabled. This exposes users of\n  "
         "this app to unnecessary security risks.\n");
  }

  // #7 Do not set allowRunningInsecureContent to true
  void WarnAboutInsecureContentAllowed(
      const blink::web_pref::WebPreferences& prefs) {
    if (!prefs.allow_running_insecure_content)
      return;
    Warn("allowRunningInsecureContent",
         "This renderer process has \"allowRunningInsecureContent\"\n  "
         "enabled. This exposes users of this app to severe security risks.\n"
         "\n  ");
  }

  // #8 Do not enable experimental features
  void WarnAboutExperimentalFeatures(const base::CommandLine& command_line) {
    if (!command_line.HasSwitch(
            ::switches::kEnableExperimentalWebPlatformFeatures)) {
      return;
    }
    Warn("experimentalFeatures",
         "This renderer process has \"experimentalFeatures\" enabled.\n  This "
         "exposes users of this app to some security risk. If you do not "
         "need\n  this feature, you should disable it.\n");
  }

  // #9 Do not use enableBlinkFeatures
  void WarnAboutEnableBlinkFeatures(const base::CommandLine& command_line) {
    if (command_line.GetSwitchValueASCII(::switches::kEnableBlinkFeatures)
            .empty()) {
      return;
    }
    Warn("enableBlinkFeatures",
         "This renderer process has additional \"enableBlinkFeatures\"\n  "
         "enabled. This exposes users of this app to some security risk. If "
         "you do not\n  need this feature, you should disable it.\n");
  }

  // #10 Do not use allowpopups
  void WarnAboutAllowedPopups() {
    if (render_frame()
            ->GetWebFrame()
            ->GetDocument()
            .QuerySelectorAll(blink::WebString::FromUtf8("[allowpopups]"))
            .empty()) {
      return;
    }
    Warn("allowpopups",
         "A <webview> has \"allowpopups\" set to true. This exposes\n    users "
         "of this app to some security risk, since popups are just\n    "
         "BrowserWindows. If you do not need this feature, you should "
         "disable it.\n\n    ");
  }

  static bool IsLocalhost(const GURL& url) {
    std::string_view host = url.host();
    return host.empty() || host == "localhost" || host == "127.0.0.1" ||
           host == "[::1]";
  }
};

}  // namespace

void MaybeAddSecurityWarnings(content::RenderFrame* render_frame) {
  // Whether to log is decided per load, since the page can opt in through
  // window.ELECTRON_ENABLE_SECURITY_WARNINGS even in a packaged app.
  if (render_frame->IsMainFrame())
    new SecurityWarnings(render_frame);
}

}  // namespace electron
