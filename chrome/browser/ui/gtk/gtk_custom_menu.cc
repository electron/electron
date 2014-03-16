// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/gtk/gtk_custom_menu.h"

#include "chrome/browser/ui/gtk/gtk_custom_menu_item.h"

G_DEFINE_TYPE(GtkCustomMenu, gtk_custom_menu, GTK_TYPE_MENU)

// Stolen directly from gtkmenushell.c. I'd love to call the library version
// instead, but it's static and isn't exported. :(
static gint gtk_menu_shell_is_item(GtkMenuShell* menu_shell,
                                   GtkWidget* child) {
  GtkWidget *parent;

  g_return_val_if_fail(GTK_IS_MENU_SHELL(menu_shell), FALSE);
  g_return_val_if_fail(child != NULL, FALSE);

  parent = gtk_widget_get_parent(child);
  while (GTK_IS_MENU_SHELL(parent)) {
    if (parent == reinterpret_cast<GtkWidget*>(menu_shell))
      return TRUE;
    parent = GTK_MENU_SHELL(parent)->parent_menu_shell;
  }

  return FALSE;
}

// Stolen directly from gtkmenushell.c. I'd love to call the library version
// instead, but it's static and isn't exported. :(
static GtkWidget* gtk_menu_shell_get_item(GtkMenuShell* menu_shell,
                                          GdkEvent* event) {
  GtkWidget* menu_item = gtk_get_event_widget(event);

  while (menu_item && !GTK_IS_MENU_ITEM(menu_item))
    menu_item = gtk_widget_get_parent(menu_item);

  if (menu_item && gtk_menu_shell_is_item(menu_shell, menu_item))
    return menu_item;
  else
    return NULL;
}

// When processing a button event, abort processing if the cursor isn't in a
// clickable region.
static gboolean gtk_custom_menu_button_press(GtkWidget* widget,
                                             GdkEventButton* event) {
  GtkWidget* menu_item = gtk_menu_shell_get_item(
      GTK_MENU_SHELL(widget), reinterpret_cast<GdkEvent*>(event));
  if (GTK_IS_CUSTOM_MENU_ITEM(menu_item)) {
    if (!gtk_custom_menu_item_is_in_clickable_region(
            GTK_CUSTOM_MENU_ITEM(menu_item))) {
      return TRUE;
    }
  }

  return GTK_WIDGET_CLASS(gtk_custom_menu_parent_class)->
      button_press_event(widget, event);
}

// When processing a button event, abort processing if the cursor isn't in a
// clickable region. If it's in a button that doesn't dismiss the menu, fire
// that event and abort having the normal GtkMenu code run.
static gboolean gtk_custom_menu_button_release(GtkWidget* widget,
                                               GdkEventButton* event) {
  GtkWidget* menu_item = gtk_menu_shell_get_item(
      GTK_MENU_SHELL(widget), reinterpret_cast<GdkEvent*>(event));
  if (GTK_IS_CUSTOM_MENU_ITEM(menu_item)) {
    if (!gtk_custom_menu_item_is_in_clickable_region(
            GTK_CUSTOM_MENU_ITEM(menu_item))) {
      // Stop processing this event. This isn't a clickable region.
      return TRUE;
    }

    if (gtk_custom_menu_item_try_no_dismiss_command(
            GTK_CUSTOM_MENU_ITEM(menu_item))) {
      return TRUE;
    }
  }

  return GTK_WIDGET_CLASS(gtk_custom_menu_parent_class)->
      button_release_event(widget, event);
}

// Manually forward button press events to the menu item (and then do what we'd
// do normally).
static gboolean gtk_custom_menu_motion_notify(GtkWidget* widget,
                                              GdkEventMotion* event) {
  GtkWidget* menu_item = gtk_menu_shell_get_item(
      GTK_MENU_SHELL(widget), (GdkEvent*)event);
  if (GTK_IS_CUSTOM_MENU_ITEM(menu_item)) {
    gtk_custom_menu_item_receive_motion_event(GTK_CUSTOM_MENU_ITEM(menu_item),
                                              event->x, event->y);
  }

  return GTK_WIDGET_CLASS(gtk_custom_menu_parent_class)->
      motion_notify_event(widget, event);
}

static void gtk_custom_menu_move_current(GtkMenuShell* menu_shell,
                                         GtkMenuDirectionType direction) {
  // If the currently selected item is custom, we give it first chance to catch
  // up/down events.

  // TODO(erg): We are breaking a GSEAL by directly accessing this. We'll need
  // to fix this by the time gtk3 comes out.
  GtkWidget* menu_item = GTK_MENU_SHELL(menu_shell)->active_menu_item;
  if (GTK_IS_CUSTOM_MENU_ITEM(menu_item)) {
    switch (direction) {
      case GTK_MENU_DIR_PREV:
      case GTK_MENU_DIR_NEXT:
        if (gtk_custom_menu_item_handle_move(GTK_CUSTOM_MENU_ITEM(menu_item),
                                             direction))
          return;
        break;
      default:
        break;
    }
  }

  GTK_MENU_SHELL_CLASS(gtk_custom_menu_parent_class)->
      move_current(menu_shell, direction);

  // In the case of hitting PREV and transitioning to a custom menu, we want to
  // make sure we're selecting the final item in the list, not the first one.
  menu_item = GTK_MENU_SHELL(menu_shell)->active_menu_item;
  if (GTK_IS_CUSTOM_MENU_ITEM(menu_item)) {
    gtk_custom_menu_item_select_item_by_direction(
        GTK_CUSTOM_MENU_ITEM(menu_item), direction);
  }
}

static void gtk_custom_menu_init(GtkCustomMenu* menu) {
}

static void gtk_custom_menu_class_init(GtkCustomMenuClass* klass) {
  GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
  GtkMenuShellClass* menu_shell_class = GTK_MENU_SHELL_CLASS(klass);

  widget_class->button_press_event = gtk_custom_menu_button_press;
  widget_class->button_release_event = gtk_custom_menu_button_release;
  widget_class->motion_notify_event = gtk_custom_menu_motion_notify;

  menu_shell_class->move_current = gtk_custom_menu_move_current;
}

GtkWidget* gtk_custom_menu_new() {
  return GTK_WIDGET(g_object_new(GTK_TYPE_CUSTOM_MENU, NULL));
}
