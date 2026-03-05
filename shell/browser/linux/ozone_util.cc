// Copyright (c) 2025 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/ozone_util.h"

#include "build/build_config.h"
#include "ui/ozone/platform_selection.h"  // nogncheck

namespace ozone_util {

bool IsX11() {
#if BUILDFLAG(IS_LINUX)
  static const bool is_x11 = ui::GetOzonePlatformId() == ui::kPlatformX11;
#else
  static const bool is_x11 = false;
#endif
  return is_x11;
}

bool IsWayland() {
#if BUILDFLAG(IS_LINUX)
  static const bool is_wayland =
      ui::GetOzonePlatformId() == ui::kPlatformWayland;
#else
  static const bool is_wayland = false;
#endif
  return is_wayland;
}

}  // namespace ozone_util
