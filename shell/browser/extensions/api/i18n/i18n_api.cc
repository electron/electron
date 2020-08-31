// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/i18n/i18n_api.h"

#include <string>
#include <vector>

#include "chrome/browser/browser_process.h"
#include "shell/common/extensions/api/i18n.h"

namespace GetAcceptLanguages = extensions::api::i18n::GetAcceptLanguages;

namespace extensions {

ExtensionFunction::ResponseAction I18nGetAcceptLanguagesFunction::Run() {
  auto locale = g_browser_process->GetApplicationLocale();
  std::vector<std::string> accept_languages = {locale};
  return RespondNow(
      ArgumentList(GetAcceptLanguages::Results::Create(accept_languages)));
}

}  // namespace extensions
