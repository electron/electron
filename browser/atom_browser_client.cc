// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_client.h"

#include "browser/atom_browser_main_parts.h"
#include "browser/net/atom_url_request_job_factory.h"
#include "content/public/common/url_constants.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "vendor/brightray/browser/url_request_context_getter.h"
#include "webkit/glue/webpreferences.h"

namespace atom {

AtomBrowserClient::AtomBrowserClient() {
}

AtomBrowserClient::~AtomBrowserClient() {
}

net::URLRequestContextGetter* AtomBrowserClient::CreateRequestContext(
    content::BrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers) {
  content::ProtocolHandlerMap preset_handlers;
  std::swap(preset_handlers, *protocol_handlers);

  // Create our implementaton of job factory.
  AtomURLRequestJobFactory* job_factory = new AtomURLRequestJobFactory;
  content::ProtocolHandlerMap::iterator it;
  for (it = preset_handlers.begin(); it != preset_handlers.end(); ++it)
    job_factory->SetProtocolHandler(it->first, it->second.release());
  job_factory->SetProtocolHandler(chrome::kDataScheme,
                                  new net::DataProtocolHandler);
  job_factory->SetProtocolHandler(chrome::kFileScheme,
                                  new net::FileProtocolHandler);

  // Go through default procedure.
  net::URLRequestContextGetter* request_context_getter =
      brightray::BrowserClient::CreateRequestContext(browser_context,
                                                     protocol_handlers);
  net::URLRequestContext* request_context =
      request_context_getter->GetURLRequestContext();

  // Replace default job factory.
  storage_.reset(new net::URLRequestContextStorage(request_context));
  storage_->set_job_factory(job_factory);

  return request_context_getter;
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
  // Restart renderer process if navigating to the same url.
  return current_url == new_url;
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new AtomBrowserMainParts;
}

}  // namespace atom
