// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "common/atom_version.h"

namespace brightray {

std::string GetApplicationName() {
  return "Atom-Shell";
}

std::string GetApplicationVersion() {
  return ATOM_VERSION_STRING;
}

}  // namespace brightray
