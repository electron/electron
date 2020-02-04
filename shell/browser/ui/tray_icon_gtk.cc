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
#include "ui/gfx/image/image_skia_operations.h"

namespace electron {

namespace {

gfx::ImageSkia GetIconFromImage(const gfx::Image& image) {
  auto icon = image.AsImageSkia();
  auto size = icon.size();

  // Systray icons are historically 22 pixels tall, e.g. on Ubuntu GNOME,
  // KDE, and xfce. Taller icons are causing incorrect sizing issues -- e.g.
  // a 1x1 icon -- so for now, pin the height manually. Similar behavior to
  // https://bugs.chromium.org/p/chromium/issues/detail?id=1042098 ?
  static constexpr int DESIRED_HEIGHT = 22;
  if ((size.height() != 0) && (size.height() != DESIRED_HEIGHT)) {
    const double ratio = DESIRED_HEIGHT / static_cast<double>(size.height());
    size = gfx::Size(static_cast<int>(ratio * size.width()),
                     static_cast<int>(ratio * size.height()));
    icon = gfx::ImageSkiaOperations::CreateResizedImage(
        icon, skia::ImageOperations::RESIZE_BEST, size);
  }

  return icon;
}

}  // namespace

TrayIconGtk::TrayIconGtk() = default;

TrayIconGtk::~TrayIconGtk() = default;

void TrayIconGtk::SetImage(const gfx::Image& image) {
  image_ = GetIconFromImage(image);

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

void TrayIconGtk::SetContextMenu(ElectronMenuModel* menu_model) {
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
