// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_TRAY_ICON_GTK_H_
#define SHELL_BROWSER_UI_TRAY_ICON_GTK_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/views/status_icons/status_icon_linux_wrapper.h"
#include "shell/browser/ui/tray_icon.h"

namespace views {
class StatusIconLinuxWrapper;
}

namespace electron {

class TrayIconGtk : public TrayIcon {
 public:
  TrayIconGtk();
  ~TrayIconGtk() override;

  // TrayIcon:
  void SetImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetContextMenu(AtomMenuModel* menu_model) override;

 private:
  std::unique_ptr<views::StatusIconLinuxWrapper> icon_wrapper_;

  gfx::ImageSkia dummy_image_;
  base::string16 dummy_string_;

  DISALLOW_COPY_AND_ASSIGN(TrayIconGtk);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_TRAY_ICON_GTK_H_
