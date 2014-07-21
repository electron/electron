// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtk2ui/gtk2_status_icon.h"

#include <gtk/gtk.h>

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/libgtk2ui/app_indicator_icon_menu.h"
#include "chrome/browser/ui/libgtk2ui/skia_utils_gtk2.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/image/image_skia.h"

namespace libgtk2ui {

Gtk2StatusIcon::Gtk2StatusIcon(const gfx::ImageSkia& image,
                               const base::string16& tool_tip) {
  GdkPixbuf* pixbuf = GdkPixbufFromSkBitmap(*image.bitmap());
  gtk_status_icon_ = gtk_status_icon_new_from_pixbuf(pixbuf);
  g_object_unref(pixbuf);

  g_signal_connect(gtk_status_icon_, "activate", G_CALLBACK(OnClickThunk),
      this);
  g_signal_connect(gtk_status_icon_, "popup_menu",
      G_CALLBACK(OnContextMenuRequestedThunk), this);
  SetToolTip(tool_tip);
}

Gtk2StatusIcon::~Gtk2StatusIcon() {
  g_object_unref(gtk_status_icon_);
}

void Gtk2StatusIcon::SetImage(const gfx::ImageSkia& image) {
  GdkPixbuf* pixbuf = GdkPixbufFromSkBitmap(*image.bitmap());
  gtk_status_icon_set_from_pixbuf(gtk_status_icon_, pixbuf);
  g_object_unref(pixbuf);
}

void Gtk2StatusIcon::SetPressedImage(const gfx::ImageSkia& image) {
  // Ignore pressed images, since the standard on Linux is to not highlight
  // pressed status icons.
}

void Gtk2StatusIcon::SetToolTip(const base::string16& tool_tip) {
  gtk_status_icon_set_tooltip_text(gtk_status_icon_,
                                   base::UTF16ToUTF8(tool_tip).c_str());
}

void Gtk2StatusIcon::UpdatePlatformContextMenu(ui::MenuModel* model) {
  menu_.reset();
  if (model)
    menu_.reset(new AppIndicatorIconMenu(model));
}

void Gtk2StatusIcon::RefreshPlatformContextMenu() {
  if (menu_.get())
    menu_->Refresh();
}

void Gtk2StatusIcon::OnClick(GtkStatusIcon* status_icon) {
  if (delegate())
    delegate()->OnClick();
}

void Gtk2StatusIcon::OnContextMenuRequested(GtkStatusIcon* status_icon,
                                            guint button,
                                            guint32 activate_time) {
  if (menu_.get()) {
    gtk_menu_popup(menu_->GetGtkMenu(),
                   NULL,
                   NULL,
                   gtk_status_icon_position_menu,
                   gtk_status_icon_,
                   button,
                   activate_time);
  }
}

}  // namespace libgtk2ui
