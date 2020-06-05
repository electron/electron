// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_GTK_GTK_STATUS_ICON_H_
#define SHELL_BROWSER_UI_GTK_GTK_STATUS_ICON_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/base/glib/glib_integers.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/views/linux_ui/status_icon_linux.h"

typedef struct _GtkStatusIcon GtkStatusIcon;

namespace gfx {
class ImageSkia;
}

namespace ui {
class MenuModel;
}

namespace electron {

namespace gtkui {
class AppIndicatorIconMenu;

// Status icon implementation which uses the system tray X11 spec (via
// GtkStatusIcon).
class GtkStatusIcon : public views::StatusIconLinux {
 public:
  GtkStatusIcon(const gfx::ImageSkia& image, const base::string16& tool_tip);
  ~GtkStatusIcon() override;

  // Overridden from views::StatusIconLinux:
  void SetIcon(const gfx::ImageSkia& image) override;
  void SetToolTip(const base::string16& tool_tip) override;
  void UpdatePlatformContextMenu(ui::MenuModel* menu) override;
  void RefreshPlatformContextMenu() override;

 private:
  CHROMEG_CALLBACK_0(GtkStatusIcon, void, OnClick, GtkStatusIcon*);

  CHROMEG_CALLBACK_2(GtkStatusIcon,
                     void,
                     OnContextMenuRequested,
                     GtkStatusIcon*,
                     guint,
                     guint);

  ::GtkStatusIcon* gtk_status_icon_;

  std::unique_ptr<AppIndicatorIconMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(GtkStatusIcon);
};

}  // namespace gtkui

}  // namespace electron

#endif  // SHELL_BROWSER_UI_GTK_GTK_STATUS_ICON_H_
