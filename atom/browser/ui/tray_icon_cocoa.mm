// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_cocoa.h"

#include "atom/browser/ui/cocoa/atom_menu_controller.h"
#include "base/strings/sys_string_conversions.h"
#include "skia/ext/skia_utils_mac.h"

namespace atom {

TrayIconCocoa::TrayIconCocoa() {
  item_.reset([[[NSStatusBar systemStatusBar]
                statusItemWithLength:NSVariableStatusItemLength] retain]);
  [item_ setEnabled:YES];
  [item_ setHighlightMode:YES];
}

TrayIconCocoa::~TrayIconCocoa() {
}

void TrayIconCocoa::SetImage(const gfx::ImageSkia& image) {
  if (!image.isNull()) {
    NSImage* ns_image = gfx::SkBitmapToNSImage(*image.bitmap());
    if (ns_image)
      [item_ setImage:ns_image];
  }
}

void TrayIconCocoa::SetPressedImage(const gfx::ImageSkia& image) {
  if (!image.isNull()) {
    NSImage* ns_image = gfx::SkBitmapToNSImage(*image.bitmap());
    if (ns_image)
      [item_ setAlternateImage:ns_image];
  }
}

void TrayIconCocoa::SetToolTip(const std::string& tool_tip) {
  [item_ setToolTip:base::SysUTF8ToNSString(tool_tip)];
}

void TrayIconCocoa::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset([[AtomMenuController alloc] initWithModel:menu_model]);
  [item_ setMenu:[menu_ menu]];
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconCocoa;
}

}  // namespace atom
