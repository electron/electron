// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_GTK_APP_INDICATOR_ICON_H_
#define ATOM_BROWSER_UI_GTK_APP_INDICATOR_ICON_H_

#include <string>

#include "atom/browser/ui/tray_icon.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "ui/base/gtk/gtk_signal.h"

typedef struct _AppIndicator AppIndicator;
typedef struct _GtkWidget GtkWidget;

class MenuGtk;

namespace atom {

class AppIndicatorIcon : public TrayIcon {
 public:
  AppIndicatorIcon();
  virtual ~AppIndicatorIcon();

  // Indicates whether libappindicator so could be opened.
  static bool CouldOpen();

  virtual void SetImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetPressedImage(const gfx::ImageSkia& image) OVERRIDE;
  virtual void SetToolTip(const std::string& tool_tip) OVERRIDE;
  virtual void SetContextMenu(ui::SimpleMenuModel* menu_model) OVERRIDE;

 private:
  void SetImageFromFile(const base::FilePath& icon_file_path);

  // Gtk status icon wrapper
  AppIndicator* icon_;

  // The context menu for this icon (if any).
  scoped_ptr<MenuGtk> menu_;

  std::string id_;
  base::FilePath icon_file_path_;
  int icon_change_count_;

  base::WeakPtrFactory<AppIndicatorIcon> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppIndicatorIcon);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_GTK_APP_INDICATOR_ICON_H_
