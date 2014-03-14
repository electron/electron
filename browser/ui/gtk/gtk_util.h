// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_GTK_GTK_UTIL_H_
#define ATOM_BROWSER_UI_GTK_GTK_UTIL_H_

#include <gtk/gtk.h>
#include <string>

namespace gtk_util {

// Left-align the given GtkMisc and return the same pointer.
GtkWidget* LeftAlignMisc(GtkWidget* misc);

// Create a left-aligned label with the given text in bold.
GtkWidget* CreateBoldLabel(const std::string& text);

// Show the image for the given menu item, even if the user's default is to not
// show images. Only to be used for favicons or other menus where the image is
// crucial to its functionality.
void SetAlwaysShowImage(GtkWidget* image_menu_item);

// Checks whether a widget is actually visible, i.e. whether it and all its
// ancestors up to its toplevel are visible.
bool IsWidgetAncestryVisible(GtkWidget* widget);

// Sets the given label's size request to |pixel_width|. This will cause the
// label to wrap if it needs to. The reason for this function is that some
// versions of GTK mis-align labels that have a size request and line wrapping,
// and this function hides the complexity of the workaround.
void SetLabelWidth(GtkWidget* label, int pixel_width);

}  // namespace gtk_util

#endif  // ATOM_BROWSER_UI_GTK_GTK_UTIL_H_
