// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_client.h"

#include "base/command_line.h"
#include "browser/atom_browser_context.h"
#include "browser/atom_browser_main_parts.h"
#include "browser/native_window.h"
#include "browser/net/atom_url_request_context_getter.h"
#include "browser/window_list.h"
#include "common/options_switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "webkit/common/webpreferences.h"

namespace atom {

namespace {

struct FindByProcessId {
  FindByProcessId(int child_process_id) : child_process_id_(child_process_id) {
  }

  bool operator() (NativeWindow* const window) {
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

net::URLRequestContextGetter* AtomBrowserClient::CreateRequestContext(
    content::BrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers) {
  return static_cast<AtomBrowserContext*>(browser_context)->
      CreateRequestContext(protocol_handlers);
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    const GURL& url,
    WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = false;
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
  prefs->experimental_webgl_enabled = false;
  prefs->allow_displaying_insecure_content = true;
  prefs->allow_running_insecure_content = true;
}

bool AtomBrowserClient::ShouldSwapProcessesForNavigation(
    content::SiteInstance* site_instance,
    const GURL& current_url,
    const GURL& new_url) {
  if (site_instance->HasProcess())
    dying_render_process_ = site_instance->GetProcess();

  // Restart renderer process for all navigations.
  return true;
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    CommandLine* command_line,
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

  // Append --node-integration to renderer process.
  if (window != NULL)
    command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                    window->node_integration());

  dying_render_process_ = NULL;
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new AtomBrowserMainParts;
}

}  // namespace atom
