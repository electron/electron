// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon.h"

namespace atom {

TrayIcon::TrayIcon() : model_(NULL) {
}

TrayIcon::~TrayIcon() {
}

void TrayIcon::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  model_ = menu_model;
}

}  // namespace atom
