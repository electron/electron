// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_gtk.h"

#include <gtk/gtk.h>

#include <algorithm>

#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/status_icons/status_icon_linux_dbus.h"
#include "shell/browser/browser.h"
#include "shell/common/application_info.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace electron {

TrayIconGtk::TrayIconGtk() = default;

TrayIconGtk::~TrayIconGtk() = default;

int GetPreferredTrayIconPixelSize() {
  static base::Optional<int> pixels;

  if (!pixels) {
    static constexpr GtkIconSize icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
    static constexpr int fallback_px = 22;
    int px;
    pixels = gtk_icon_size_lookup(icon_size, &px, nullptr) ? px : fallback_px;
  }

  return *pixels;
}

gfx::ImageSkia GetIconFromImage(const gfx::Image& image) {
  auto icon = image.AsImageSkia();
  auto size = icon.size();

  // Ensure that the icon is no larger than the preferred size.
  // Works around (GNOME shell?) issue which warps icon if icon exceeds that
  // size. #21445
  const int max_size = GetPreferredTrayIconPixelSize();
  if (!size.IsEmpty() &&
      ((size.width() > max_size) || (size.height() > max_size))) {
    const double ratio =
        max_size / double(std::max(size.width(), size.height()));
    size = gfx::Size(int(ratio * size.width()), int(ratio * size.height()));
    icon = gfx::ImageSkiaOperations::CreateResizedImage(
        icon, skia::ImageOperations::RESIZE_BEST, size);
  }

  return icon;
}

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

void TrayIconGtk::SetContextMenu(AtomMenuModel* menu_model) {
  icon_->UpdatePlatformContextMenu(menu_model_);
  menu_model_ = menu_model;
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
