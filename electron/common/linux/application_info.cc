// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "electron/common/electron_version.h"

namespace brightray {

std::string GetApplicationName() {
  return ELECTRON_PRODUCT_NAME;
}

std::string GetApplicationVersion() {
  return ELECTRON_VERSION_STRING;
}

}  // namespace brightray
