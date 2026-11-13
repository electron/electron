// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_linux.h"

#include "base/check_op.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/views/status_icons/status_icon_linux_dbus.h"
#include "shell/browser/browser.h"
#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/status_icon_gtk.h"
#include "ui/gfx/image/image_skia.h"
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

TrayIconLinux::TrayIconLinux()
    : status_icon_dbus_(new StatusIconLinuxDbus(Browser::Get()->GetName())) {
  status_icon_dbus_->SetDelegate(this);
}

TrayIconLinux::~TrayIconLinux() {
  if (auto* status_icon = GetStatusIcon())
    status_icon->SetDelegate(nullptr);
  // The D-Bus icon may still be waiting for a StatusNotifierWatcher.
  if (status_icon_dbus_)
    status_icon_dbus_->SetDelegate(nullptr);
}

void TrayIconLinux::SetImage(const gfx::Image& image) {
  image_ = GetBestImageRep(image.AsImageSkia());
  if (auto* status_icon = GetStatusIcon())
    status_icon->SetImage(image_);
}

void TrayIconLinux::SetToolTip(const std::string& tool_tip) {
  tool_tip_ = base::UTF8ToUTF16(tool_tip);
  if (auto* status_icon = GetStatusIcon())
    status_icon->SetToolTip(tool_tip_);
}

void TrayIconLinux::SetContextMenu(raw_ptr<ElectronMenuModel> menu_model) {
  menu_model_ = menu_model;
  if (auto* status_icon = GetStatusIcon())
    status_icon->UpdatePlatformContextMenu(menu_model_);
}

const gfx::ImageSkia& TrayIconLinux::GetImage() const {
  return image_;
}

const gfx::VectorIcon* TrayIconLinux::GetIcon() const {
  return nullptr;
}

const std::u16string& TrayIconLinux::GetToolTip() const {
  return tool_tip_;
}

ui::MenuModel* TrayIconLinux::GetMenuModel() const {
  return menu_model_;
}

void TrayIconLinux::OnImplInitializationFailed() {
  switch (status_icon_type_) {
    case StatusIconType::kDbus:
      status_icon_dbus_.reset();
      status_icon_gtk_ = std::make_unique<StatusIconGtk>();
      status_icon_type_ = StatusIconType::kGtk;
      status_icon_gtk_->SetDelegate(this);
      return;
    case StatusIconType::kGtk:
      status_icon_gtk_.reset();
      status_icon_type_ = StatusIconType::kNone;
      // Keep the menu for the D-Bus icon, which may still initialize.
      if (status_icon_dbus_)
        return;
      menu_model_ = nullptr;
      return;
    case StatusIconType::kNone:
      NOTREACHED();
  }
}

void TrayIconLinux::OnImplInitializationDelayed() {
  // Only the D-Bus icon delays initialization. Keep it, and use the GTK icon
  // until it initializes.
  CHECK_EQ(status_icon_type_, StatusIconType::kDbus);
  status_icon_gtk_ = std::make_unique<StatusIconGtk>();
  status_icon_type_ = StatusIconType::kGtk;
  status_icon_gtk_->SetDelegate(this);
}

void TrayIconLinux::OnImplInitialized() {
  if (status_icon_type_ == StatusIconType::kDbus || !status_icon_dbus_)
    return;

  // The D-Bus icon initialized after a delay. Replace the fallback with it, and
  // update it with any changes made since it read its initial state.
  status_icon_gtk_.reset();
  status_icon_type_ = StatusIconType::kDbus;
  status_icon_dbus_->UpdatePlatformContextMenu(menu_model_);
  status_icon_dbus_->SetToolTip(tool_tip_);
  status_icon_dbus_->SetImage(image_);
}

void TrayIconLinux::OnClick() {
  NotifyClicked();
}

bool TrayIconLinux::HasClickAction() {
  // Returning true will make the tooltip show as an additional context menu
  // item, which makes sense in Chrome but not in most Electron apps.
  return false;
}

ui::StatusIconLinux* TrayIconLinux::GetStatusIcon() {
  switch (status_icon_type_) {
    case StatusIconType::kDbus:
      return status_icon_dbus_.get();
    case StatusIconType::kGtk:
      return status_icon_gtk_.get();
    case StatusIconType::kNone:
      return nullptr;
  }
}

// static
TrayIcon* TrayIcon::Create(std::optional<base::Uuid> guid) {
  return new TrayIconLinux;
}

}  // namespace electron
