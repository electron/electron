// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_GTK2_STATUS_ICON_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_GTK2_STATUS_ICON_H_

#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_signal.h"
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

namespace libgtk2ui {
class AppIndicatorIconMenu;

// Status icon implementation which uses the system tray X11 spec (via
// GtkStatusIcon).
class Gtk2StatusIcon : public views::StatusIconLinux {
 public:
  Gtk2StatusIcon(const gfx::ImageSkia& image, const base::string16& tool_tip);
  virtual ~Gtk2StatusIcon();

  // Overridden from views::StatusIconLinux:
  virtual void SetImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetPressedImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetToolTip(const base::string16& tool_tip) OVERRIDE;
  virtual void UpdatePlatformContextMenu(ui::MenuModel* menu) OVERRIDE;
  virtual void RefreshPlatformContextMenu() OVERRIDE;

 private:
  CHROMEG_CALLBACK_0(Gtk2StatusIcon, void, OnClick, GtkStatusIcon*);

  CHROMEG_CALLBACK_2(Gtk2StatusIcon,
                     void,
                     OnContextMenuRequested,
                     GtkStatusIcon*,
                     guint,
                     guint);

  GtkStatusIcon* gtk_status_icon_;

  scoped_ptr<AppIndicatorIconMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(Gtk2StatusIcon);
};

}  // namespace libgtk2ui

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_GTK2_STATUS_ICON_H_
