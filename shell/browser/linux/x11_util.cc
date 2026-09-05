// Copyright (c) 2025 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/x11_util.h"

#include "build/build_config.h"
#include "ui/ozone/platform_selection.h"  // nogncheck

namespace x11_util {

bool IsX11() {
#if BUILDFLAG(IS_LINUX)
  static const bool is = ui::GetOzonePlatformId() == ui::kPlatformX11;
  return is;
#else
  return false;
#endif
}

bool IsWayland() {
#if BUILDFLAG(IS_LINUX)
  static const bool is = ui::GetOzonePlatformId() == ui::kPlatformWayland;
  return is;
#else
  return false;
#endif
}

}  // namespace x11_util
