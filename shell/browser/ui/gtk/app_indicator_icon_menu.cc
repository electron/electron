// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk/app_indicator_icon_menu.h"

#include <gtk/gtk.h>

#include "base/bind.h"
#include "base/debug/leak_annotations.h"
#include "shell/browser/ui/gtk/menu_util.h"
#include "ui/base/models/menu_model.h"

namespace electron::gtkui {

AppIndicatorIconMenu::AppIndicatorIconMenu(ui::MenuModel* model)
    : menu_model_(model) {
  {
    ANNOTATE_SCOPED_MEMORY_LEAK;  // http://crbug.com/378770
    gtk_menu_ = gtk_menu_new();
  }
  g_object_ref_sink(gtk_menu_);
  if (menu_model_) {
    BuildSubmenuFromModel(menu_model_, gtk_menu_,
                          G_CALLBACK(OnMenuItemActivatedThunk),
                          &block_activation_, this);
    Refresh();
  }
}

AppIndicatorIconMenu::~AppIndicatorIconMenu() {
  gtk_widget_destroy(gtk_menu_);
  g_object_unref(gtk_menu_);
}

void AppIndicatorIconMenu::UpdateClickActionReplacementMenuItem(
    const char* label,
    const base::RepeatingClosure& callback) {
  click_action_replacement_callback_ = callback;

  if (click_action_replacement_menu_item_added_) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(gtk_menu_));
    for (GList* child = children; child; child = g_list_next(child)) {
      if (g_object_get_data(G_OBJECT(child->data), "click-action-item") !=
          nullptr) {
        gtk_menu_item_set_label(GTK_MENU_ITEM(child->data), label);
        break;
      }
    }
    g_list_free(children);
  } else {
    click_action_replacement_menu_item_added_ = true;

    // If |menu_model_| is non empty, add a separator to separate the
    // "click action replacement menu item" from the other menu items.
    if (menu_model_ && menu_model_->GetItemCount() > 0) {
      GtkWidget* menu_item = gtk_separator_menu_item_new();
      gtk_widget_show(menu_item);
      gtk_menu_shell_prepend(GTK_MENU_SHELL(gtk_menu_), menu_item);
    }

    GtkWidget* menu_item = gtk_menu_item_new_with_mnemonic(label);
    g_object_set_data(G_OBJECT(menu_item), "click-action-item",
                      GINT_TO_POINTER(1));
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(OnClickActionReplacementMenuItemActivatedThunk),
                     this);
    gtk_widget_show(menu_item);
    gtk_menu_shell_prepend(GTK_MENU_SHELL(gtk_menu_), menu_item);
  }
}

void AppIndicatorIconMenu::Refresh() {
  gtk_container_foreach(GTK_CONTAINER(gtk_menu_), SetMenuItemInfo,
                        &block_activation_);
}

GtkMenu* AppIndicatorIconMenu::GetGtkMenu() {
  return GTK_MENU(gtk_menu_);
}

void AppIndicatorIconMenu::OnClickActionReplacementMenuItemActivated(
    GtkWidget* menu_item) {
  click_action_replacement_callback_.Run();
}

void AppIndicatorIconMenu::OnMenuItemActivated(GtkWidget* menu_item) {
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
