// Copyright (c) 2025 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/ozone_util.h"

#include "ui/ozone/platform_selection.h"  // nogncheck

namespace ozone_util {

bool IsX11() {
  static const bool is_x11 = ui::GetOzonePlatformId() == ui::kPlatformX11;
  return is_x11;
}

}  // namespace ozone_util
