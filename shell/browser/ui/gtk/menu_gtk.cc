// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk/menu_gtk.h"

#include <gtk/gtk.h>

#include "base/functional/bind.h"
#include "shell/browser/ui/gtk/menu_util.h"
#include "ui/base/models/menu_model.h"

namespace electron::gtkui {

MenuGtk::MenuGtk(ui::MenuModel* model)
    : menu_model_(model), gtk_menu_(TakeGObject(gtk_menu_new())) {
  if (menu_model_) {
    BuildSubmenuFromModel(menu_model_, gtk_menu_,
                          base::BindRepeating(&MenuGtk::OnMenuItemActivated,
                                              base::Unretained(this)),
                          &block_activation_, &signals_);
    Refresh();
  }
}

MenuGtk::~MenuGtk() {
  gtk_widget_destroy(gtk_menu_);
}

void MenuGtk::Refresh() {
  gtk_container_foreach(GTK_CONTAINER(gtk_menu_.get()), SetMenuItemInfo,
                        &block_activation_);
}

GtkMenu* MenuGtk::GetGtkMenu() {
  return GTK_MENU(gtk_menu_.get());
}

void MenuGtk::OnMenuItemActivated(GtkWidget* menu_item) {
  if (block_activation_)
    return;

  ui::MenuModel* model = ModelForMenuItem(GTK_MENU_ITEM(menu_item));
  if (!model) {
    // There won't be a model for "native" submenus like the "Input Methods"
    // context menu. We don't need to handle activation messages for submenus
    // anyway, so we can just return here.
    DCHECK(gtk_menu_item_get_submenu(GTK_MENU_ITEM(menu_item)));
    return;
  }

  // The activate signal is sent to radio items as they get deselected;
  // ignore it in this case.
  if (GTK_IS_RADIO_MENU_ITEM(menu_item) &&
      !gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu_item))) {
    return;
  }

  int id;
  if (!GetMenuItemID(menu_item, &id))
    return;

  // The menu item can still be activated by hotkeys even if it is disabled.
  if (model->IsEnabledAt(id))
    ExecuteCommand(model, id);
}

}  // namespace electron::gtkui
