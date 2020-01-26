// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_gtk.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/status_icons/status_icon_linux_dbus.h"
#include "shell/browser/browser.h"
#include "shell/common/application_info.h"
#include "ui/gfx/image/image.h"

namespace electron {

TrayIconGtk::TrayIconGtk() = default;

TrayIconGtk::~TrayIconGtk() = default;

void TrayIconGtk::SetImage(const gfx::Image& image) {
  image_ = image.AsImageSkia();
  if (icon_) {
    icon_->SetIcon(image_);
    return;
  }

  tool_tip_ = base::UTF8ToUTF16(GetApplicationName());

  icon_ = base::MakeRefCounted<StatusIconLinuxDbus>();
  icon_->SetIcon(image_);
  icon_->SetToolTip(tool_tip_);
  icon_->SetDelegate(this);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  tool_tip_ = base::UTF8ToUTF16(tool_tip);
  icon_->SetToolTip(tool_tip_);
}

void TrayIconGtk::SetContextMenu(AtomMenuModel* menu_model) {
  menu_model_ = menu_model;
  icon_->UpdatePlatformContextMenu(menu_model_);
}

const gfx::ImageSkia& TrayIconGtk::GetImage() const {
  return image_;
}

const base::string16& TrayIconGtk::GetToolTip() const {
  return tool_tip_;
}

ui::MenuModel* TrayIconGtk::GetMenuModel() const {
  return menu_model_;
}

void TrayIconGtk::OnImplInitializationFailed() {}

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

}  // namespace electron
