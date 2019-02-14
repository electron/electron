// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents.h"

#include "base/strings/sys_string_conversions.h"
#include "base/win/i18n.h"
#include "content/public/common/renderer_preferences.h"

namespace atom {

namespace api {

void WebContents::SetAcceptLanguages(content::RendererPreferences* prefs) {
  std::vector<base::string16> languages;
  base::win::i18n::GetThreadPreferredUILanguageList(&languages);
  std::string accept_langs;
  for (const auto& s16 : languages) {
    accept_langs += base::SysWideToUTF8(s16) + ',';
  }
  accept_langs.pop_back();
  prefs->accept_languages = accept_langs;
}

}  // namespace api

}  // namespace atom