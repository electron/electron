// Copyright (c) 2023 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_STATUS_ICON_GTK_H_
#define ELECTRON_SHELL_BROWSER_UI_STATUS_ICON_GTK_H_

#include <memory>

#include "ui/base/glib/glib_integers.h"
#include "ui/base/glib/scoped_gobject.h"
#include "ui/base/glib/scoped_gsignal.h"
#include "ui/linux/status_icon_linux.h"

typedef struct _GtkStatusIcon GtkStatusIcon;

namespace electron {

namespace gtkui {
class MenuGtk;
}

class StatusIconGtk : public ui::StatusIconLinux {
 public:
  StatusIconGtk();
  StatusIconGtk(const StatusIconGtk&) = delete;
  StatusIconGtk& operator=(const StatusIconGtk&) = delete;
  ~StatusIconGtk() override;

  // ui::StatusIconLinux:
  void SetIcon(const gfx::ImageSkia& image) override;
  void SetToolTip(const std::u16string& tool_tip) override;
  void UpdatePlatformContextMenu(ui::MenuModel* model) override;
  void RefreshPlatformContextMenu() override;
  void OnSetDelegate() override;

 private:
  void OnClick(GtkStatusIcon* status_icon);
  void OnContextMenuRequested(GtkStatusIcon* status_icon,
                              guint button,
                              guint32 activate_time);

  std::unique_ptr<gtkui::MenuGtk> menu_;
  ScopedGObject<GtkStatusIcon> icon_;
  std::vector<ScopedGSignal> signals_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_STATUS_ICON_GTK_H_
