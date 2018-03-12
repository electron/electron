// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/common/util.h"

#include "base/environment.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"

namespace util {

bool GetDesktopName(std::string* setme) {
  bool found = false;

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string desktop_id = libgtkui::GetDesktopName(env.get());
  constexpr char const* libcc_default_id = "chromium-browser.desktop";
  if (!desktop_id.empty() && (desktop_id != libcc_default_id)) {
    *setme = desktop_id;
    found = true;
  }

  return found;
}

}  // namespace util
