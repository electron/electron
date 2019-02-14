// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "chrome/browser/browser_process.h"
#include "content/public/common/renderer_preferences.h"

namespace atom {

namespace api {

void WebContents::SetAcceptLanguages(content::RendererPreferences* prefs) {
  prefs->accept_languages = g_browser_process->GetApplicationLocale();
}

}  // namespace api

}  // namespace atom