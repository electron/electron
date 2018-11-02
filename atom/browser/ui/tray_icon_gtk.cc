// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_gtk.h"

#include "atom/browser/browser.h"
#include "atom/common/application_info.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace {

// Number of app indicators used (used as part of app-indicator id).
int indicators_count;

}  // namespace

TrayIconGtk::TrayIconGtk() {}

TrayIconGtk::~TrayIconGtk() {}

void TrayIconGtk::SetImage(const gfx::Image& image) {
  if (icon_) {
    icon_->SetImage(image.AsImageSkia());
    return;
  }

  const auto toolTip = base::UTF8ToUTF16(GetApplicationName());
  icon_.reset(views::LinuxUI::instance()->CreateLinuxStatusIcon(
      image.AsImageSkia(), toolTip));
  icon_->set_delegate(this);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  icon_->SetToolTip(base::UTF8ToUTF16(tool_tip));
}

void TrayIconGtk::SetContextMenu(AtomMenuModel* menu_model) {
  icon_->UpdatePlatformContextMenu(menu_model);
}

void TrayIconGtk::OnClick() {
  NotifyClicked();
}

bool TrayIconGtk::HasClickAction() {
  return false;
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconGtk;
}

}  // namespace atom
