// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_H_
#define CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_H_

// GtkCustomMenu is a GtkMenu subclass that can contain, and collaborates with,
// GtkCustomMenuItem instances. GtkCustomMenuItem is a GtkMenuItem that can
// have buttons and other normal widgets embeded in it. GtkCustomMenu exists
// only to override most of the button/motion/move callback functions so
// that the normal GtkMenu implementation doesn't handle events related to
// GtkCustomMenuItem items.
//
// For a more through overview of this system, see the comments in
// gtk_custom_menu_item.h.

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_CUSTOM_MENU                                            \
  (gtk_custom_menu_get_type())
#define GTK_CUSTOM_MENU(obj)                                            \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_CUSTOM_MENU, GtkCustomMenu))
#define GTK_CUSTOM_MENU_CLASS(klass)                                    \
  (G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_CUSTOM_MENU, GtkCustomMenuClass))
#define GTK_IS_CUSTOM_MENU(obj)                                         \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_CUSTOM_MENU))
#define GTK_IS_CUSTOM_MENU_CLASS(klass)                                 \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_CUSTOM_MENU))
#define GTK_CUSTOM_MENU_GET_CLASS(obj)                                  \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GTK_TYPE_CUSTOM_MENU, GtkCustomMenuClass))

typedef struct _GtkCustomMenu GtkCustomMenu;
typedef struct _GtkCustomMenuClass GtkCustomMenuClass;

struct _GtkCustomMenu {
  GtkMenu menu;
};

struct _GtkCustomMenuClass {
  GtkMenuClass parent_class;
};

GType gtk_custom_menu_get_type(void) G_GNUC_CONST;
GtkWidget* gtk_custom_menu_new();

G_END_DECLS

#endif  // CHROME_BROWSER_UI_GTK_GTK_CUSTOM_MENU_H_
