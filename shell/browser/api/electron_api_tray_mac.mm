// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "shell/browser/api/electron_api_tray.h"
#include "shell/browser/ui/tray_icon_cocoa.h"
#include "ui/gfx/image/image.h"

namespace electron::api {

void SetLayeredTrayImages(TrayIcon* tray_icon,
                          const std::vector<gfx::Image>& layers) {
  electron::TrayIconCocoa* cocoa_tray =
      static_cast<electron::TrayIconCocoa*>(tray_icon);
  cocoa_tray->SetLayeredImages(layers);
}

void SetPressedLayeredTrayImages(TrayIcon* tray_icon,
                                 const std::vector<gfx::Image>& layers) {
  electron::TrayIconCocoa* cocoa_tray =
      static_cast<electron::TrayIconCocoa*>(tray_icon);
  cocoa_tray->SetAlternateLayeredImages(layers);
}

}  // namespace electron::api
