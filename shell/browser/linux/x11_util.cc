// Copyright (c) 2025 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/linux/x11_util.h"

#include "ui/ozone/public/ozone_platform.h"

namespace x11_util {

bool IsX11() {
  return ui::OzonePlatform::GetInstance()
      ->GetPlatformProperties()
      .skia_can_fall_back_to_x11;
}

}  // namespace x11_util
