// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_GTK_MENU_GTK_H_
#define ELECTRON_SHELL_BROWSER_UI_GTK_MENU_GTK_H_

#include "base/memory/raw_ptr.h"
#include "ui/base/glib/scoped_gobject.h"
#include "ui/base/glib/scoped_gsignal.h"

typedef struct _GtkMenu GtkMenu;
typedef struct _GtkWidget GtkWidget;

namespace ui {
class MenuModel;
}

namespace electron::gtkui {

class MenuGtk {
 public:
  explicit MenuGtk(ui::MenuModel* model);
  virtual ~MenuGtk();

  // Refreshes all the menu item labels and menu item checked/enabled states.
  void Refresh();

  GtkMenu* GetGtkMenu();

 private:
  // Callback for when a menu item is activated.
  void OnMenuItemActivated(GtkWidget* menu_item);

  raw_ptr<ui::MenuModel> menu_model_;  // not owned
  ScopedGObject<GtkWidget> gtk_menu_;

  bool block_activation_ = false;
  std::vector<ScopedGSignal> signals_;
};

}  // namespace electron::gtkui

#endif  // ELECTRON_SHELL_BROWSER_UI_GTK_MENU_GTK_H_
