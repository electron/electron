// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_TRAY_ICON_GTK_H_
#define ATOM_BROWSER_UI_TRAY_ICON_GTK_H_

#include <string>

#include "atom/browser/ui/tray_icon.h"
#include "ui/views/linux_ui/status_icon_linux.h"

namespace views {
class StatusIconLinux;
}

namespace atom {

class TrayIconGtk : public TrayIcon,
                    public views::StatusIconLinux::Delegate {
 public:
  TrayIconGtk();
  virtual ~TrayIconGtk();

  // TrayIcon:
  void SetImage(const gfx::Image& image) override;
  void SetToolTip(const std::string& tool_tip) override;
  void SetContextMenu(ui::MenuModel* menu_model) override;

 private:
  // views::StatusIconLinux::Delegate:
  void OnClick() override;
  bool HasClickAction() override;

  std::unique_ptr<views::StatusIconLinux> icon_;

  DISALLOW_COPY_AND_ASSIGN(TrayIconGtk);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_TRAY_ICON_GTK_H_
