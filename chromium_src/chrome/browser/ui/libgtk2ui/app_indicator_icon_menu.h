// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_APP_INDICATOR_ICON_MENU_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_APP_INDICATOR_ICON_MENU_H_

#include "base/callback.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_signal.h"

typedef struct _GtkMenu GtkMenu;
typedef struct _GtkWidget GtkWidget;

namespace ui {
class MenuModel;
}

namespace libgtk2ui {

// The app indicator icon's menu.
class AppIndicatorIconMenu {
 public:
  explicit AppIndicatorIconMenu(ui::MenuModel* model);
  virtual ~AppIndicatorIconMenu();

  // Sets a menu item at the top of |gtk_menu_| as a replacement for the app
  // indicator icon's click action. |callback| is called when the menu item
  // is activated.
  void UpdateClickActionReplacementMenuItem(const char* label,
                                            const base::Closure& callback);

  // Refreshes all the menu item labels and menu item checked/enabled states.
  void Refresh();

  GtkMenu* GetGtkMenu();

 private:
  // Callback for when the "click action replacement" menu item is activated.
  CHROMEGTK_CALLBACK_0(AppIndicatorIconMenu,
                       void,
                       OnClickActionReplacementMenuItemActivated);

  // Callback for when a menu item is activated.
  CHROMEGTK_CALLBACK_0(AppIndicatorIconMenu, void, OnMenuItemActivated);

  // Not owned.
  ui::MenuModel* menu_model_;

  // Whether a "click action replacement" menu item has been added to the menu.
  bool click_action_replacement_menu_item_added_;

  // Called when the click action replacement menu item is activated. When a
  // menu item from |menu_model_| is activated, MenuModel::ActivatedAt() is
  // invoked and is assumed to do any necessary processing.
  base::Closure click_action_replacement_callback_;

  GtkWidget* gtk_menu_;

  bool block_activation_;

  DISALLOW_COPY_AND_ASSIGN(AppIndicatorIconMenu);
};

}  // namespace libgtk2ui

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_APP_INDICATOR_ICON_MENU_H_
