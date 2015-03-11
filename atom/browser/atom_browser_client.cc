// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#include "atom/browser/atom_access_token_store.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_view_manager.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "ui/base/l10n/l10n_util.h"

namespace atom {

namespace {

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

AtomBrowserClient::AtomBrowserClient()
    : dying_render_process_(NULL) {
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
    const GURL& url,
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
  if (url.SchemeIs("chrome-devtools")) {
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

  NativeWindow* window = NativeWindow::FromRenderView(
      process->GetID(), render_view_host->GetRoutingID());
  if (window)
    window->OverrideWebkitPrefs(url, prefs);
}

bool AtomBrowserClient::ShouldSwapBrowsingInstancesForNavigation(
    content::SiteInstance* site_instance,
    const GURL& current_url,
    const GURL& new_url) {
  if (site_instance->HasProcess())
    dying_render_process_ = site_instance->GetProcess();

  // Restart renderer process for all navigations, this relies on a patch to
  // Chromium: http://git.io/_PaNyg.
  return true;
}

std::string AtomBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  WindowList* list = WindowList::GetInstance();
  NativeWindow* window = NULL;

  // Find the owner of this child process.
  WindowList::const_iterator iter = std::find_if(
      list->begin(), list->end(), FindByProcessId(child_process_id));
  if (iter != list->end())
    window = *iter;

  // If the render process is a newly started one, which means the window still
  // uses the old going-to-be-swapped render process, then we try to find the
  // window from the swapped render process.
  if (window == NULL && dying_render_process_ != NULL) {
    child_process_id = dying_render_process_->GetID();
    WindowList::const_iterator iter = std::find_if(
        list->begin(), list->end(), FindByProcessId(child_process_id));
    if (iter != list->end())
      window = *iter;
  }

  if (window != NULL) {
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

  dying_render_process_ = NULL;
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  v8::V8::Initialize();  // Init V8 before creating main parts.
  return new AtomBrowserMainParts;
}

}  // namespace atom
