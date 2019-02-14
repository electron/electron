// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/application_info.h"

#include "atom/browser/browser.h"
#include "atom/common/atom_version.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "chrome/common/chrome_version.h"
#include "content/public/common/user_agent.h"

namespace atom {

namespace {

base::NoDestructor<std::string> g_overridden_application_name;
base::NoDestructor<std::string> g_overridden_application_version;

}  // namespace

// name
void OverrideApplicationName(const std::string& name) {
  *g_overridden_application_name = name;
}
std::string GetOverriddenApplicationName() {
  return *g_overridden_application_name;
}

// version
void OverrideApplicationVersion(const std::string& version) {
  *g_overridden_application_version = version;
}
std::string GetOverriddenApplicationVersion() {
  return *g_overridden_application_version;
}

std::string GetApplicationUserAgent() {
  // Construct user agent string.
  Browser* browser = Browser::Get();
  std::string name, user_agent;
  if (!base::RemoveChars(browser->GetName(), " ", &name))
    name = browser->GetName();
  if (name == ATOM_PRODUCT_NAME) {
    user_agent = "Chrome/" CHROME_VERSION_STRING " " ATOM_PRODUCT_NAME
                 "/" ATOM_VERSION_STRING;
  } else {
    user_agent = base::StringPrintf(
        "%s/%s Chrome/%s " ATOM_PRODUCT_NAME "/" ATOM_VERSION_STRING,
        name.c_str(), browser->GetVersion().c_str(), CHROME_VERSION_STRING);
  }
  return content::BuildUserAgentFromProduct(user_agent);
}

}  // namespace atom
