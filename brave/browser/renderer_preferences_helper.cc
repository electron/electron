// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/renderer_preferences_helper.h"

#include "brave/browser/brave_content_browser_client.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"

using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(brave::RendererPreferencesHelper);

namespace brave {

RendererPreferencesHelper::RendererPreferencesHelper(WebContents* contents)
    : content::WebContentsObserver(contents) {
  UpdateRendererPreferences();
}

RendererPreferencesHelper::~RendererPreferencesHelper() {
}

void RendererPreferencesHelper::UpdateRendererPreferences() {
  content::RendererPreferences* prefs =
      web_contents()->GetMutableRendererPrefs();

  auto browser_context = web_contents()->GetBrowserContext();
  std::string accept_lang = static_cast<BraveContentBrowserClient*>(
      BraveContentBrowserClient::Get())->GetAcceptLangs(browser_context);
  prefs->accept_languages = accept_lang;
  web_contents()->GetRenderViewHost()->SyncRendererPrefs();
}

void RendererPreferencesHelper::RenderViewHostChanged(
                                            content::RenderViewHost* old_host,
                                            content::RenderViewHost* new_host) {
  UpdateRendererPreferences();
}

} // namespace brave
