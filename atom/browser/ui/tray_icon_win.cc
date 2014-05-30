// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_win.h"

namespace atom {

TrayIconWin::TrayIconWin() {
}

TrayIconWin::~TrayIconWin() {
}

void TrayIconWin::SetImage(const gfx::ImageSkia& image) {
}

void TrayIconWin::SetPressedImage(const gfx::ImageSkia& image) {
}

void TrayIconWin::SetToolTip(const std::string& tool_tip) {
}

void TrayIconWin::SetContextMenu(ui::SimpleMenuModel* menu_model) {
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconWin;
}

}  // namespace atom
