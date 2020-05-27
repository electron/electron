// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/language_util.h"

#include "ui/base/l10n/l10n_util.h"

namespace electron {

std::vector<std::string> GetPreferredLanguages() {
  // Return empty as there's no API to use. You may be able to use
  // GetApplicationLocale() of a browser process.
  return std::vector<std::string>{};
}

}  // namespace electron
