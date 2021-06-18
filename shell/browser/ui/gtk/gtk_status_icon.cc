// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/gtk/gtk_status_icon.h"

#include <gtk/gtk.h>

#include "base/debug/leak_annotations.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/ui/gtk/app_indicator_icon_menu.h"
#include "shell/browser/ui/gtk_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/image/image_skia.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

namespace electron {

namespace gtkui {

GtkStatusIcon::GtkStatusIcon(const gfx::ImageSkia& image,
                             const std::u16string& tool_tip) {
  GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(*image.bitmap());
  {
    // GTK has a bug that leaks 384 bytes when creating a GtkStatusIcon.  It
    // will not be fixed since the status icon was deprecated in version 3.14.
    // Luckily, Chromium doesn't need to create a status icon very often, if at
    // all.
    ANNOTATE_SCOPED_MEMORY_LEAK;
    gtk_status_icon_ = gtk_status_icon_new_from_pixbuf(pixbuf);
  }
  g_object_unref(pixbuf);

  g_signal_connect(gtk_status_icon_, "activate", G_CALLBACK(OnClickThunk),
                   this);
  g_signal_connect(gtk_status_icon_, "popup_menu",
                   G_CALLBACK(OnContextMenuRequestedThunk), this);
  SetToolTip(tool_tip);
}

GtkStatusIcon::~GtkStatusIcon() {
  gtk_status_icon_set_visible(gtk_status_icon_, FALSE);
  g_object_unref(gtk_status_icon_);
}

void GtkStatusIcon::SetIcon(const gfx::ImageSkia& image) {
  GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(*image.bitmap());
  gtk_status_icon_set_from_pixbuf(gtk_status_icon_, pixbuf);
  g_object_unref(pixbuf);
}

void GtkStatusIcon::SetToolTip(const std::u16string& tool_tip) {
  gtk_status_icon_set_tooltip_text(gtk_status_icon_,
                                   base::UTF16ToUTF8(tool_tip).c_str());
}

void GtkStatusIcon::UpdatePlatformContextMenu(ui::MenuModel* model) {
  menu_.reset();
  if (model)
    menu_ = std::make_unique<AppIndicatorIconMenu>(model);
}

void GtkStatusIcon::RefreshPlatformContextMenu() {
  if (menu_.get())
    menu_->Refresh();
}

void GtkStatusIcon::OnClick(GtkStatusIcon* status_icon) {
  if (delegate())
    delegate()->OnClick();
}

void GtkStatusIcon::OnContextMenuRequested(GtkStatusIcon* status_icon,
                                           guint button,
                                           guint32 activate_time) {
  if (menu_.get()) {
    gtk_menu_popup(menu_->GetGtkMenu(), nullptr, nullptr,
                   gtk_status_icon_position_menu, gtk_status_icon_, button,
                   activate_time);
  }
}

}  // namespace gtkui

}  // namespace electron
