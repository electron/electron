// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon_gtk.h"

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/browser.h"
#include "shell/common/application_info.h"
#include "ui/gfx/image/image.h"
#include "ui/views/linux_ui/linux_ui.h"

namespace electron {

TrayIconGtk::TrayIconGtk() = default;

TrayIconGtk::~TrayIconGtk() = default;

void TrayIconGtk::SetImage(const gfx::Image& image) {
  if (icon_wrapper_) {
    icon_wrapper_->SetImage(image.AsImageSkia());
    return;
  }

  const auto toolTip = base::UTF8ToUTF16(GetApplicationName());
  icon_wrapper_ = views::CreateWrappedStatusIcon(image.AsImageSkia(), tool_tip);
}

void TrayIconGtk::SetToolTip(const std::string& tool_tip) {
  icon_wrapper_->SetToolTip(base::UTF8ToUTF16(tool_tip));
}

void TrayIconGtk::SetContextMenu(AtomMenuModel* menu_model) {
  icon_wrapper_->UpdatePlatformContextMenu(menu_model);
}

void TrayIconGtk::OnImplInitializationFailed() {}

void TrayIconGtk::OnClick() {
  icon_wrapper_->OnClick();
}

bool TrayIconGtk::HasClickAction() {
  return false;
}

// static
TrayIcon* TrayIcon::Create() {
  return new TrayIconGtk;
}

}  // namespace electron
