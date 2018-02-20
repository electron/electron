// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brightray/common/application_info.h"

#include <string>

#include "atom/common/atom_version.h"
#include "base/log.h"

namespace brightray {

std::string GetApplicationName() {
  return ATOM_PRODUCT_NAME;
}

std::string GetApplicationVersion() {
  std::string ret;

  // ensure ATOM_PRODUCT_NAME and ATOM_PRODUCT_STRING match up
  if (GetApplicationName() == ATOM_PRODUCT_NAME)
    ret = ATOM_VERSION_STRING;

  // try to use the string set in app.setVersion()
  if (ret.empty())
    ret = GetOverriddenApplicationVersion();

  // no known version number; return some safe fallback
  if (ret.empty()) {
    LOG(WARNING) << "No version found. Was app.setVersion() called?";
    ret = "0.0";
  }

  return ret;
}

}  // namespace brightray
