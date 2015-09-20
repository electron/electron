// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon_gtk.h"

#include "base/guid.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/libgtk2ui/app_indicator_icon.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_status_icon.h"
#include "ui/gfx/image/image.h"

namespace atom {

TrayIconGtk::TrayIconGtk() {
}

TrayIconGtk::~TrayIconGtk() {
}

void TrayIconGtk::SetImage(const gfx::Image& image) {
  if (icon_) {
    icon_->SetImage(image.AsImageSkia());
    return;
  }

  base::string16 empty;
  if (libgtk2ui::AppIndicatorIcon::CouldOpen())
    icon_.reset(new libgtk2ui::AppIndicatorIcon(
        base::GenerateGUID(), image.AsImageSkia(), empty));
  else
    icon_.reset(new libgtk2ui::Gtk2StatusIcon(image.AsImageSkia(), empty));
  icon_->set_delegate(this);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  icon_->SetToolTip(base::UTF8ToUTF16(tool_tip));
}

void TrayIconGtk::SetContextMenu(ui::SimpleMenuModel* menu_model) {
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
