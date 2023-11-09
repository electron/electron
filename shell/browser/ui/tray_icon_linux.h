// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_LINUX_H_
#define ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_LINUX_H_

#include <memory>
#include <string>

#include "shell/browser/ui/tray_icon.h"
#include "ui/linux/status_icon_linux.h"

class StatusIconLinuxDbus;

namespace electron {

class StatusIconGtk;

class TrayIconLinux : public TrayIcon, public ui::StatusIconLinux::Delegate {
 public:
  TrayIconLinux();
  ~TrayIconLinux() override;

  // TrayIcon:
  void SetImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetContextMenu(raw_ptr<ElectronMenuModel> menu_model) override;

  // ui::StatusIconLinux::Delegate
  void OnClick() override;
  bool HasClickAction() override;
  const gfx::ImageSkia& GetImage() const override;
  const std::u16string& GetToolTip() const override;
  ui::MenuModel* GetMenuModel() const override;
  void OnImplInitializationFailed() override;

 private:
  enum class StatusIconType {
    kDbus,
    kGtk,
    kNone,
  };

  ui::StatusIconLinux* GetStatusIcon();

  scoped_refptr<StatusIconLinuxDbus> status_icon_dbus_;
  std::unique_ptr<StatusIconGtk> status_icon_gtk_;
  StatusIconType status_icon_type_;

  gfx::ImageSkia image_;
  std::u16string tool_tip_;
  raw_ptr<ui::MenuModel> menu_model_ = nullptr;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_TRAY_ICON_LINUX_H_
