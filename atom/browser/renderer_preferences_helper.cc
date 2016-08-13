// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/browser/renderer_preferences_helper.h"

#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"

using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(RendererPreferencesHelper);

namespace atom {

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
  std::string accept_lang = static_cast<brave::BraveContentBrowserClient*>(
      brave::BraveContentBrowserClient::Get())->GetAcceptLangs(browser_context);
  web_contents_->GetRenderViewHost()->SyncRendererPrefs();
}

}  // namespace atom
