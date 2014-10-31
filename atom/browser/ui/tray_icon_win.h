// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_TRAY_ICON_WIN_H_
#define ATOM_BROWSER_UI_TRAY_ICON_WIN_H_

#include <string>

#include "atom/browser/ui/tray_icon.h"

namespace atom {

class TrayIconWin : public TrayIcon {
 public:
  TrayIconWin();
  virtual ~TrayIconWin();

  virtual void SetImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetPressedImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetToolTip(const std::string& tool_tip) OVERRIDE;
  virtual void SetContextMenu(ui::SimpleMenuModel* menu_model) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayIconWin);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_TRAY_ICON_WIN_H_
