// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_GTK_STATUS_ICON_H_
#define ATOM_BROWSER_UI_GTK_STATUS_ICON_H_

#include <gtk/gtk.h>

#include <string>

#include "atom/browser/ui/tray_icon.h"
#include "ui/base/gtk/gtk_signal.h"

class MenuGtk;

namespace atom {

class StatusIcon : public TrayIcon {
 public:
  StatusIcon();
  virtual ~StatusIcon();

  virtual void SetImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetPressedImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetToolTip(const std::string& tool_tip) OVERRIDE;
  virtual void SetContextMenu(ui::SimpleMenuModel* menu_model) OVERRIDE;

 private:
  // Callback invoked when user right-clicks on the status icon.
  CHROMEGTK_CALLBACK_2(StatusIcon, void, OnPopupMenu, guint, guint);

  // Callback invoked when the icon is clicked.
  CHROMEGTK_CALLBACK_0(StatusIcon, void, OnActivate);

  // The currently-displayed icon for the window.
  GtkStatusIcon* icon_;

  // The context menu for this icon (if any).
  scoped_ptr<MenuGtk> menu_;

  DISALLOW_COPY_AND_ASSIGN(StatusIcon);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_GTK_STATUS_ICON_H_
