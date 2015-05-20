// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#include "atom/browser/atom_access_token_store.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_quota_permission_context.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_view_manager.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "ppapi/host/ppapi_host.h"
#include "ui/base/l10n/l10n_util.h"

namespace atom {

namespace {

// Next navigation should not restart renderer process.
bool g_suppress_renderer_process_restart = false;

struct FindByProcessId {
  explicit FindByProcessId(int child_process_id)
      : child_process_id_(child_process_id) {
  }

  bool operator() (NativeWindow* const window) {
    content::WebContents* web_contents = window->GetWebContents();
    if (!web_contents)
      return false;

    int id = window->GetWebContents()->GetRenderProcessHost()->GetID();
    return id == child_process_id_;
  }

  int child_process_id_;
};

}  // namespace

// static
void AtomBrowserClient::SuppressRendererProcessRestartForOnce() {
  g_suppress_renderer_process_restart = true;
}

AtomBrowserClient::AtomBrowserClient()
    : dying_render_process_(nullptr) {
}

AtomBrowserClient::~AtomBrowserClient() {
}

void AtomBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  host->AddFilter(new printing::PrintingMessageFilter(host->GetID()));
  host->AddFilter(new TtsMessageFilter(id, host->GetBrowserContext()));
}

content::SpeechRecognitionManagerDelegate*
    AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

content::AccessTokenStore* AtomBrowserClient::CreateAccessTokenStore() {
  return new AtomAccessTokenStore;
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_delegate_.reset(new AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_delegate_.get());
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->javascript_can_open_windows_automatically = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->java_enabled = false;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = true;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_universal_access_from_file_urls = true;
  prefs->allow_file_access_from_file_urls = true;
  prefs->experimental_webgl_enabled = true;
  prefs->allow_displaying_insecure_content = false;
  prefs->allow_running_insecure_content = false;

  // Turn off web security for devtools.
  auto web_contents = content::WebContents::FromRenderViewHost(
      render_view_host);
  if (web_contents && web_contents->GetURL().SchemeIs("chrome-devtools")) {
    prefs->web_security_enabled = false;
    return;
  }

  // Custom preferences of guest page.
  auto process = render_view_host->GetProcess();
  WebViewManager::WebViewInfo info;
  if (WebViewManager::GetInfoForProcess(process, &info)) {
    prefs->web_security_enabled = !info.disable_web_security;
    return;
  }

  NativeWindow* window = NativeWindow::FromWebContents(web_contents);
  if (window)
    window->OverrideWebkitPrefs(prefs);
}

std::string AtomBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void AtomBrowserClient::OverrideSiteInstanceForNavigation(
    content::BrowserContext* browser_context,
    content::SiteInstance* current_instance,
    const GURL& url,
    content::SiteInstance** new_instance) {
  if (g_suppress_renderer_process_restart) {
    g_suppress_renderer_process_restart = false;
    return;
  }

  if (current_instance->HasProcess())
    dying_render_process_ = current_instance->GetProcess();

  // Restart renderer process for all navigations.
  *new_instance = content::SiteInstance::CreateForURL(browser_context, url);
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  std::string process_type = command_line->GetSwitchValueASCII("type");
  if (process_type != "renderer")
    return;

  WindowList* list = WindowList::GetInstance();
  NativeWindow* window = nullptr;

  // Find the owner of this child process.
  WindowList::const_iterator iter = std::find_if(
      list->begin(), list->end(), FindByProcessId(child_process_id));
  if (iter != list->end())
    window = *iter;

  // If the render process is a newly started one, which means the window still
  // uses the old going-to-be-swapped render process, then we try to find the
  // window from the swapped render process.
  if (!window && dying_render_process_) {
    int dying_process_id = dying_render_process_->GetID();
    WindowList::const_iterator iter = std::find_if(
        list->begin(), list->end(), FindByProcessId(dying_process_id));
    if (iter != list->end()) {
      window = *iter;
      child_process_id = dying_process_id;
    } else {
      // It appears that the dying process doesn't belong to a BrowserWindow,
      // then it might be a guest process, if it is we should update its
      // process ID in the WebViewManager.
      auto child_process = content::RenderProcessHost::FromID(child_process_id);
      // Update the process ID in webview guests.
      WebViewManager::UpdateGuestProcessID(dying_render_process_,
                                           child_process);
    }
  }

  if (window) {
    window->AppendExtraCommandLineSwitches(command_line, child_process_id);
  } else {
    // Append commnad line arguments for guest web view.
    auto child_process = content::RenderProcessHost::FromID(child_process_id);
    WebViewManager::WebViewInfo info;
    if (WebViewManager::GetInfoForProcess(child_process, &info)) {
      command_line->AppendSwitchASCII(
          switches::kGuestInstanceID,
          base::IntToString(info.guest_instance_id));
      command_line->AppendSwitchASCII(
          switches::kNodeIntegration,
          info.node_integration ? "true" : "false");
      if (info.plugins)
        command_line->AppendSwitch(switches::kEnablePlugins);
      if (!info.preload_script.empty())
        command_line->AppendSwitchPath(
            switches::kPreloadScript,
            info.preload_script);
    }
  }

  dying_render_process_ = nullptr;
}

void AtomBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
  auto command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnablePlugins))
    browser_host->GetPpapiHost()->AddHostFactoryFilter(
        scoped_ptr<ppapi::host::HostFactory>(
            new chrome::ChromeBrowserPepperHostFactory(browser_host)));
}

content::QuotaPermissionContext*
    AtomBrowserClient::CreateQuotaPermissionContext() {
  return new AtomQuotaPermissionContext;
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  v8::V8::Initialize();  // Init V8 before creating main parts.
  return new AtomBrowserMainParts;
}

}  // namespace atom
