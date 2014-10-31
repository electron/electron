// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon.h"

namespace atom {

TrayIcon::TrayIcon() {
}

TrayIcon::~TrayIcon() {
}

void TrayIcon::SetTitle(const std::string& title) {
}

void TrayIcon::SetHighlightMode(bool highlight) {
}

void TrayIcon::NotifyClicked() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnClicked());
}

void TrayIcon::NotifyDoubleClicked() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDoubleClicked());
}

}  // namespace atom
