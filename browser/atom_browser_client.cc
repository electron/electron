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
#include "content/public/browser/web_contents.h"
#include "webkit/common/webpreferences.h"

namespace atom {

AtomBrowserClient::AtomBrowserClient() {
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
  // Restart renderer process for all navigations.
  return true;
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    CommandLine* command_line,
    int child_process_id) {
  // Append --node-integration to renderer process.
  WindowList* list = WindowList::GetInstance();
  for (WindowList::const_iterator iter = list->begin(); iter != list->end();
       ++iter) {
    NativeWindow* window = *iter;
    int id = window->GetWebContents()->GetRenderProcessHost()->GetID();
    if (id == child_process_id) {
      command_line->AppendSwitchASCII(switches::kNodeIntegration,
                                      window->node_integration());
      return;
    }
  }
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new AtomBrowserMainParts;
}

}  // namespace atom
