// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_client.h"

#include "atom/browser/atom_access_token_store.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "webkit/common/webpreferences.h"

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
  host->AddFilter(new PrintingMessageFilter(host->GetID()));
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_delegate_.reset(new AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_delegate_.get());
}

content::AccessTokenStore* AtomBrowserClient::CreateAccessTokenStore() {
  return new AtomAccessTokenStore;
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    const GURL& url,
    WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->javascript_can_open_windows_automatically = true;
  prefs->plugins_enabled = false;
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
  prefs->allow_displaying_insecure_content = true;
  prefs->allow_running_insecure_content = true;

  // Turn off web security for devtools.
  if (url.SchemeIs("chrome-devtools")) {
    prefs->web_security_enabled = false;
    return;
  }

  NativeWindow* window = NativeWindow::FromRenderView(
      render_view_host->GetProcess()->GetID(),
      render_view_host->GetRoutingID());
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

  if (window != NULL)
    window->AppendExtraCommandLineSwitches(command_line, child_process_id);

  dying_render_process_ = NULL;
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  v8::V8::Initialize();  // Init V8 before creating main parts.
  return new AtomBrowserMainParts;
}

}  // namespace atom
