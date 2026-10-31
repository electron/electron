// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/app_shim_constants.h"

#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/chrome_version.h"
#include "crypto/hash.h"
#include "electron/electron_version.h"

namespace electron::app_shim {

std::string BuildMarkerServerName(const std::string& server_name) {
  // Mojo message IDs are scrambled per Chromium version, and Electron's own
  // interface with the shim can change with every Electron version.
  const auto build =
      crypto::hash::Sha256(ELECTRON_VERSION_STRING "/" CHROME_VERSION_STRING);
  return base::StrCat(
      {server_name, ".", base::HexEncode(base::span(build).first<8>())});
}

}  // namespace electron::app_shim
