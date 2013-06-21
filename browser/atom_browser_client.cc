// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_client.h"

#include "browser/atom_browser_main_parts.h"
#include "browser/media/media_capture_devices_dispatcher.h"
#include "webkit/glue/webpreferences.h"

namespace atom {

AtomBrowserClient::AtomBrowserClient() {
}

AtomBrowserClient::~AtomBrowserClient() {
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

content::MediaObserver* AtomBrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
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
