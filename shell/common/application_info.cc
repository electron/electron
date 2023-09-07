// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/application_info.h"

#include "base/i18n/rtl.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_version.h"
#include "content/public/common/user_agent.h"
#include "electron/electron_version.h"
#include "shell/browser/browser.h"

namespace electron {

std::string& OverriddenApplicationName() {
  static base::NoDestructor<std::string> overridden_application_name;
  return *overridden_application_name;
}

std::string& OverriddenApplicationVersion() {
  static base::NoDestructor<std::string> overridden_application_version;
  return *overridden_application_version;
}

std::string GetPossiblyOverriddenApplicationName() {
  std::string ret = OverriddenApplicationName();
  if (!ret.empty())
    return ret;
  return GetApplicationName();
}

std::string GetApplicationUserAgent() {
  // Construct user agent string.
  Browser* browser = Browser::Get();
  std::string name, user_agent;
  if (!base::RemoveChars(browser->GetName(), " ", &name))
    name = browser->GetName();
  if (name == ELECTRON_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " ELECTRON_PRODUCT_NAME
                 "/" ELECTRON_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ELECTRON_PRODUCT_NAME "/" ELECTRON_VERSION_STRING,
        name.c_str(), browser->GetVersion().c_str(), CHROME_VERSION_STRING);
  }
  return content::BuildUserAgentFromProduct(user_agent);
}

bool IsAppRTL() {
  const std::string& locale = g_browser_process->GetApplicationLocale();
  base::i18n::TextDirection text_direction =
      base::i18n::GetTextDirectionForLocaleInStartUp(locale.c_str());
  return text_direction == base::i18n::RIGHT_TO_LEFT;
}

}  // namespace electron
