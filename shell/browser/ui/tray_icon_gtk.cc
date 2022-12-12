// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_gtk.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/status_icons/status_icon_linux_dbus.h"
#include "ui/gfx/image/image_skia_rep.h"

namespace electron {

namespace {

gfx::ImageSkia GetBestImageRep(const gfx::ImageSkia& image) {
  image.EnsureRepsForSupportedScales();
  float best_scale = 0.0f;
  SkBitmap best_rep;
  for (const auto& rep : image.image_reps()) {
    if (rep.scale() > best_scale) {
      best_scale = rep.scale();
      best_rep = rep.GetBitmap();
    }
  }
  // All status icon implementations want the image in pixel coordinates, so use
  // a scale factor of 1.
  return gfx::ImageSkia::CreateFromBitmap(best_rep, 1.0f);
}

}  // namespace

TrayIconGtk::TrayIconGtk()
    : status_icon_(new StatusIconLinuxDbus),
      status_icon_type_(StatusIconType::kDbus) {
  status_icon_->SetDelegate(this);
}

TrayIconGtk::~TrayIconGtk() = default;

void TrayIconGtk::SetImage(const gfx::Image& image) {
  image_ = GetBestImageRep(image.AsImageSkia());
  if (status_icon_)
    status_icon_->SetIcon(image_);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  tool_tip_ = base::UTF8ToUTF16(tool_tip);
  if (status_icon_)
    status_icon_->SetToolTip(tool_tip_);
}

void TrayIconGtk::SetContextMenu(ElectronMenuModel* menu_model) {
  menu_model_ = menu_model;
  if (status_icon_)
    status_icon_->UpdatePlatformContextMenu(menu_model_);
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

void TrayIconGtk::OnImplInitializationFailed() {
  switch (status_icon_type_) {
    case StatusIconType::kDbus:
      status_icon_ = nullptr;
      status_icon_type_ = StatusIconType::kNone;
      return;
    case StatusIconType::kNone:
      NOTREACHED();
  }
}

void TrayIconGtk::OnClick() {
  NotifyClicked();
}

bool TrayIconGtk::HasClickAction() {
  // Returning true will make the tooltip show as an additional context menu
  // item, which makes sense in Chrome but not in most Electron apps.
  return false;
}

// static
TrayIcon* TrayIcon::Create(absl::optional<UUID> guid) {
  return new TrayIconGtk;
}

}  // namespace electron
