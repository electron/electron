// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_TRAY_ICON_H_
#define ATOM_BROWSER_UI_TRAY_ICON_H_

#include <string>

#include "ui/base/models/simple_menu_model.h"

namespace atom {

class TrayIcon {
 public:
  static TrayIcon* Create();

  virtual ~TrayIcon();

  // Sets the image associated with this status icon.
  virtual void SetImage(const gfx::ImageSkia& image) = 0;

  // Sets the image associated with this status icon when pressed.
  virtual void SetPressedImage(const gfx::ImageSkia& image) = 0;

  // Sets the hover text for this status icon. This is also used as the label
  // for the menu item which is created as a replacement for the status icon
  // click action on platforms that do not support custom click actions for the
  // status icon (e.g. Ubuntu Unity).
  virtual void SetToolTip(const std::string& tool_tip) = 0;

  // Set the context menu for this icon.
  virtual void SetContextMenu(ui::SimpleMenuModel* menu_model) = 0;

 protected:
  TrayIcon();

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayIcon);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_TRAY_ICON_H_
