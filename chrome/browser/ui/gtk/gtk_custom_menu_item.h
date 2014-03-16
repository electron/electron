// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_ITEM_H_
#define CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_ITEM_H_

// GtkCustomMenuItem is a GtkMenuItem subclass that has buttons in it and acts
// to support this. GtkCustomMenuItems only render properly when put in a
// GtkCustomMenu; there's a lot of collaboration between these two classes
// necessary to work around how gtk normally does menus.
//
// We can't rely on the normal event infrastructure. While a menu is up, the
// GtkMenu has a grab on all events. Instead of trying to pump events through
// the normal channels, we have the GtkCustomMenu selectively forward mouse
// motion through a back channel. The GtkCustomMenu only listens for button
// press information so it can block the effects of the click if the cursor
// isn't in a button in the menu item.
//
// A GtkCustomMenuItem doesn't try to take these signals and forward them to
// the buttons it owns. The GtkCustomMenu class keeps track of which button is
// selected (due to key events and mouse movement) and otherwise acts like a
// normal GtkItem. The buttons are only for sizing and rendering; they don't
// respond to events. Instead, when the GtkCustomMenuItem is activated by the
// GtkMenu, it uses which button was selected as a signal of what to do.
//
// Users should connect to the "button-pushed" signal to be notified when a
// button was pushed. We don't go through the normal "activate" signal because
// we need to communicate additional information, namely which button was
// activated.

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_CUSTOM_MENU_ITEM                                       \
  (gtk_custom_menu_item_get_type())
#define GTK_CUSTOM_MENU_ITEM(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_CUSTOM_MENU_ITEM,         \
                              GtkCustomMenuItem))
#define GTK_CUSTOM_MENU_ITEM_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_CUSTOM_MENU_ITEM,          \
                           GtkCustomMenuItemClass))
#define GTK_IS_CUSTOM_MENU_ITEM(obj)                                    \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_CUSTOM_MENU_ITEM))
#define GTK_IS_CUSTOM_MENU_ITEM_CLASS(klass)                            \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_CUSTOM_MENU_ITEM))
#define GTK_CUSTOM_MENU_ITEM_GET_CLASS(obj)                             \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_CUSTOM_MENU_ITEM,          \
                             GtkCustomMenuItemClass))

typedef struct _GtkCustomMenuItem GtkCustomMenuItem;
typedef struct _GtkCustomMenuItemClass GtkCustomMenuItemClass;

struct _GtkCustomMenuItem {
  GtkMenuItem menu_item;

  // Container for button widgets.
  GtkWidget* hbox;

  // Label on left side of menu item.
  GtkWidget* label;

  // List of all widgets we added. Used to find the leftmost and rightmost
  // continuous buttons.
  GList* all_widgets;

  // Possible button widgets. Used for keyboard navigation.
  GList* button_widgets;

  // The widget that currently has highlight.
  GtkWidget* currently_selected_button;

  // The widget that was selected *before* |currently_selected_button|. Why do
  // we hang on to this? Because the menu system sends us a deselect signal
  // right before activating us. We need to listen to deselect since that's
  // what we receive when the mouse cursor leaves us entirely.
  GtkWidget* previously_selected_button;
};

struct _GtkCustomMenuItemClass {
  GtkMenuItemClass parent_class;
};

GType gtk_custom_menu_item_get_type(void) G_GNUC_CONST;
GtkWidget* gtk_custom_menu_item_new(const char* title);

// Adds a button to our list of items in the |hbox|.
GtkWidget* gtk_custom_menu_item_add_button(GtkCustomMenuItem* menu_item,
                                           int command_id);

// Adds a button to our list of items in the |hbox|, but that isn't part of
// |button_widgets| to prevent it from being activatable.
GtkWidget* gtk_custom_menu_item_add_button_label(GtkCustomMenuItem* menu_item,
                                                 int command_id);

// Adds a vertical space in the |hbox|.
void gtk_custom_menu_item_add_space(GtkCustomMenuItem* menu_item);

// Receives a motion event from the GtkCustomMenu that contains us. We can't
// just subscribe to motion-event or the individual widget enter/leave events
// because the top level GtkMenu has an event grab.
void gtk_custom_menu_item_receive_motion_event(GtkCustomMenuItem* menu_item,
                                               gdouble x, gdouble y);

// Notification that the menu got a cursor key event. Used to move up/down
// within the menu buttons. Returns TRUE to stop the default signal handler
// from running.
gboolean gtk_custom_menu_item_handle_move(GtkCustomMenuItem* menu_item,
                                          GtkMenuDirectionType direction);

// Because we only get a generic "selected" signal when we've changed, we need
// to have a way for the GtkCustomMenu to tell us that we were just
// selected.
void gtk_custom_menu_item_select_item_by_direction(
    GtkCustomMenuItem* menu_item, GtkMenuDirectionType direction);

// Whether we are currently hovering over a clickable region on the menu
// item. Used by GtkCustomMenu to determine whether it should discard click
// events.
gboolean gtk_custom_menu_item_is_in_clickable_region(
    GtkCustomMenuItem* menu_item);

// If the button is released while the |currently_selected_button| isn't
// supposed to dismiss the menu, this signals to our listeners that we want to
// run this command if it doesn't dismiss the menu.  Returns TRUE if we acted
// on this button click (and should prevent the normal GtkMenu machinery from
// firing an "activate" signal).
gboolean gtk_custom_menu_item_try_no_dismiss_command(
    GtkCustomMenuItem* menu_item);

// Calls |callback| with every button and button-label in the container.
void gtk_custom_menu_item_foreach_button(GtkCustomMenuItem* menu_item,
                                         GtkCallback callback,
                                         gpointer callback_data);

G_END_DECLS

#endif  // CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_ITEM_H_
