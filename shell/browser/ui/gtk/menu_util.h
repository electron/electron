// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_GTK_MENU_UTIL_H_
#define ELECTRON_SHELL_BROWSER_UI_GTK_MENU_UTIL_H_

#include <gtk/gtk.h>

#include <string>

#include "ui/gfx/image/image.h"

namespace ui {
class MenuModel;
}

namespace electron::gtkui {

// Builds GtkImageMenuItems.
GtkWidget* BuildMenuItemWithImage(const std::string& label, GtkWidget* image);
GtkWidget* BuildMenuItemWithImage(const std::string& label,
                                  const gfx::Image& icon);
GtkWidget* BuildMenuItemWithLabel(const std::string& label);

ui::MenuModel* ModelForMenuItem(GtkMenuItem* menu_item);

// This method is used to build the menu dynamically. The return value is the
// new menu item.
GtkWidget* AppendMenuItemToMenu(int index,
                                ui::MenuModel* model,
                                GtkWidget* menu_item,
                                GtkWidget* menu,
                                bool connect_to_activate,
                                GCallback item_activated_cb,
                                void* this_ptr);

// Gets the ID of a menu item.
// Returns true if the menu item has an ID.
bool GetMenuItemID(GtkWidget* menu_item, int* menu_id);

// Execute command associated with specified id.
void ExecuteCommand(ui::MenuModel* model, int id);

// Creates a GtkMenu from |model_|. block_activation_ptr is used to disable
// the item_activated_callback while we set up the set up the menu items.
// See comments in definition of SetMenuItemInfo for more info.
void BuildSubmenuFromModel(ui::MenuModel* model,
                           GtkWidget* menu,
                           GCallback item_activated_cb,
                           bool* block_activation,
                           void* this_ptr);

// Sets the check mark, enabled/disabled state and dynamic labels on menu items.
void SetMenuItemInfo(GtkWidget* widget, void* block_activation_ptr);

}  // namespace electron::gtkui

#endif  // ELECTRON_SHELL_BROWSER_UI_GTK_MENU_UTIL_H_
