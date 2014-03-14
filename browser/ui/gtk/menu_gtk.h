// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_GTK_MENU_GTK_H_
#define ATOM_BROWSER_UI_GTK_MENU_GTK_H_

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "ui/base/gtk/gtk_signal.h"
#include "ui/base/gtk/gtk_signal_registrar.h"
#include "ui/gfx/point.h"

namespace gfx {
class Image;
}

namespace ui {
class ButtonMenuItemModel;
class MenuModel;
}

class MenuGtk {
 public:
  // Delegate class that lets another class control the status of the menu.
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called before a command is executed. This exists for the case where a
    // model is handling the actual execution of commands, but the delegate
    // still needs to know that some command got executed. This is called before
    // and not after the command is executed because its execution may delete
    // the menu and/or the delegate.
    virtual void CommandWillBeExecuted() {}

    // Called when the menu stops showing. This will be called before
    // ExecuteCommand if the user clicks an item, but will also be called when
    // the user clicks away from the menu.
    virtual void StoppedShowing() {}

    // Return true if we should override the "gtk-menu-images" system setting
    // when showing image menu items for this menu.
    virtual bool AlwaysShowIconForCmd(int command_id) const;

    // Returns a tinted image used in button in a menu.
    virtual GtkIconSet* GetIconSetForId(int idr);

    // Returns an icon for the menu item, if available.
    virtual GtkWidget* GetImageForCommandId(int command_id) const;

    static GtkWidget* GetDefaultImageForLabel(const std::string& label);
  };

  MenuGtk(MenuGtk::Delegate* delegate,
          ui::MenuModel* model,
          bool is_menubar = false);
  virtual ~MenuGtk();

  // Initialize GTK signal handlers.
  void ConnectSignalHandlers();

  // These methods are used to build the menu dynamically. The return value
  // is the new menu item.
  GtkWidget* AppendMenuItemWithLabel(int command_id, const std::string& label);
  GtkWidget* AppendMenuItemWithIcon(int command_id, const std::string& label,
                                    const gfx::Image& icon);
  GtkWidget* AppendCheckMenuItemWithLabel(int command_id,
                                          const std::string& label);
  GtkWidget* AppendSeparator();
  GtkWidget* InsertSeparator(int position);
  GtkWidget* AppendMenuItem(int command_id, GtkWidget* menu_item);
  GtkWidget* InsertMenuItem(int command_id, GtkWidget* menu_item, int position);
  GtkWidget* AppendMenuItemToMenu(int index,
                                  ui::MenuModel* model,
                                  GtkWidget* menu_item,
                                  GtkWidget* menu,
                                  bool connect_to_activate);
  GtkWidget* InsertMenuItemToMenu(int index,
                                  ui::MenuModel* model,
                                  GtkWidget* menu_item,
                                  GtkWidget* menu,
                                  int position,
                                  bool connect_to_activate);

  // Displays the menu near a widget, as if the widget were a menu bar.
  // Example: the wrench menu button.
  // |button| is the mouse button that brought up the menu.
  // |event_time| is the time from the GdkEvent.
  void PopupForWidget(GtkWidget* widget, int button, guint32 event_time);

  // Displays the menu as a context menu, i.e. at the cursor location.
  // It is implicit that it was brought up using the right mouse button.
  // |point| is the point where to put the menu.
  // |event_time| is the time of the event that triggered the menu's display.
  void PopupAsContext(const gfx::Point& point, guint32 event_time);

  // Displays the menu as a context menu for the passed status icon.
  void PopupAsContextForStatusIcon(guint32 event_time, guint32 button,
                                   GtkStatusIcon* icon);

  // Displays the menu following a keyboard event (such as selecting |widget|
  // and pressing "enter").
  void PopupAsFromKeyEvent(GtkWidget* widget);

  // Closes the menu.
  void Cancel();

  // Repositions the menu to be right under the button.  Alignment is set as
  // object data on |void_widget| with the tag "left_align".  If "left_align"
  // is true, it aligns the left side of the menu with the left side of the
  // button. Otherwise it aligns the right side of the menu with the right side
  // of the button. Public since some menus have odd requirements that don't
  // belong in a public class.
  static void WidgetMenuPositionFunc(GtkMenu* menu,
                                     int* x,
                                     int* y,
                                     gboolean* push_in,
                                     void* void_widget);

  // Positions the menu to appear at the gfx::Point represented by |userdata|.
  static void PointMenuPositionFunc(GtkMenu* menu,
                                    int* x,
                                    int* y,
                                    gboolean* push_in,
                                    gpointer userdata);

  GtkWidget* widget() const { return menu_; }

  // Updates all the enabled/checked states and the dynamic labels.
  void UpdateMenu();

 private:
  // Builds a GtkImageMenuItem.
  GtkWidget* BuildMenuItemWithImage(const std::string& label,
                                    const gfx::Image& icon);

  GtkWidget* BuildMenuItemWithImage(const std::string& label,
                                    GtkWidget* image);

  GtkWidget* BuildMenuItemWithLabel(const std::string& label,
                                    int command_id);

  // A function that creates a GtkMenu from |model_|.
  void BuildMenuFromModel();
  // Implementation of the above; called recursively.
  void BuildSubmenuFromModel(ui::MenuModel* model, GtkWidget* menu);
  // Builds a menu item with buttons in it from the data in the model.
  GtkWidget* BuildButtonMenuItem(ui::ButtonMenuItemModel* model,
                                 GtkWidget* menu);

  void ExecuteCommand(ui::MenuModel* model, int id);

  // Callback for when a menu item is clicked.
  CHROMEGTK_CALLBACK_0(MenuGtk, void, OnMenuItemActivated);

  // Called when one of the buttons is pressed.
  CHROMEGTK_CALLBACK_1(MenuGtk, void, OnMenuButtonPressed, int);

  // Called to maybe activate a button if that button isn't supposed to dismiss
  // the menu.
  CHROMEGTK_CALLBACK_1(MenuGtk, gboolean, OnMenuTryButtonPressed, int);

  // Updates all the menu items' state.
  CHROMEGTK_CALLBACK_0(MenuGtk, void, OnMenuShow);

  // Sets the activating widget back to a normal appearance.
  CHROMEGTK_CALLBACK_0(MenuGtk, void, OnMenuHidden);

  // Focus out event handler for the menu.
  CHROMEGTK_CALLBACK_1(MenuGtk, gboolean, OnMenuFocusOut, GdkEventFocus*);

  // Handles building dynamic submenus on demand when they are shown.
  CHROMEGTK_CALLBACK_0(MenuGtk, void, OnSubMenuShow);

  // Handles trearing down dynamic submenus when they have been closed.
  CHROMEGTK_CALLBACK_0(MenuGtk, void, OnSubMenuHidden);

  // Scheduled by OnSubMenuHidden() to avoid deleting submenus when hidden
  // before pending activations within them are delivered.
  static void OnSubMenuHiddenCallback(GtkWidget* submenu);

  // Sets the enable/disabled state and dynamic labels on our menu items.
  static void SetButtonItemInfo(GtkWidget* button, gpointer userdata);

  // Sets the check mark, enabled/disabled state and dynamic labels on our menu
  // items.
  static void SetMenuItemInfo(GtkWidget* widget, void* raw_menu);

  // Queries this object about the menu state.
  MenuGtk::Delegate* delegate_;

  // If non-NULL, the MenuModel that we use to populate and control the GTK
  // menu (overriding the delegate as a controller).
  ui::MenuModel* model_;

  // Whether this is a menu bar.
  bool is_menubar_;

  // For some menu items, we want to show the accelerator, but not actually
  // explicitly handle it. To this end we connect those menu items' accelerators
  // to this group, but don't attach this group to any top level window.
  GtkAccelGroup* dummy_accel_group_;

  // gtk_menu_popup() does not appear to take ownership of popup menus, so
  // MenuGtk explicitly manages the lifetime of the menu.
  GtkWidget* menu_;

  // True when we should ignore "activate" signals.  Used to prevent
  // menu items from getting activated when we are setting up the
  // menu.
  static bool block_activation_;

  ui::GtkSignalRegistrar signal_;

  base::WeakPtrFactory<MenuGtk> weak_factory_;
};

#endif  // ATOM_BROWSER_UI_GTK_MENU_GTK_H_
