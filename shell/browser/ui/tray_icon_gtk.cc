// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_gtk.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/browser.h"
#include "shell/browser/ui/gtk/status_icon.h"
#include "shell/common/application_info.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"

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

  icon_ = gtkui::CreateLinuxStatusIcon(image_, tool_tip_,
                                       Browser::Get()->GetName().c_str());
  icon_->SetDelegate(this);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  tool_tip_ = base::UTF8ToUTF16(tool_tip);
  icon_->SetToolTip(tool_tip_);
}

void TrayIconGtk::SetContextMenu(ElectronMenuModel* menu_model) {
  menu_model_ = menu_model;
  icon_->UpdatePlatformContextMenu(menu_model_);
}

const gfx::ImageSkia& TrayIconGtk::GetImage() const {
  return image_;
}

const std::u16string& TrayIconGtk::GetToolTip() const {
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
TrayIcon* TrayIcon::Create(absl::optional<UUID> guid) {
  return new TrayIconGtk;
}

}  // namespace electron
