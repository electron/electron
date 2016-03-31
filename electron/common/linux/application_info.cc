// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/atom_version.h"

namespace brightray {

std::string GetApplicationName() {
  return ATOM_PRODUCT_NAME;
}

std::string GetApplicationVersion() {
  return ATOM_VERSION_STRING;
}

}  // namespace brightray
