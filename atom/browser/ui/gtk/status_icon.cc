// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/gtk/status_icon.h"

#include "chrome/browser/ui/gtk/menu_gtk.h"
#include "ui/gfx/gtk_util.h"

namespace atom {

StatusIcon::StatusIcon() : icon_(gtk_status_icon_new()) {
  gtk_status_icon_set_visible(icon_, TRUE);

  g_signal_connect(icon_, "activate", G_CALLBACK(OnActivateThunk), this);
  g_signal_connect(icon_, "popup-menu", G_CALLBACK(OnPopupMenuThunk), this);
}

StatusIcon::~StatusIcon() {
  gtk_status_icon_set_visible(icon_, FALSE);
  g_object_unref(icon_);
}

void StatusIcon::SetImage(const gfx::ImageSkia& image) {
  if (image.isNull())
    return;

  GdkPixbuf* pixbuf = gfx::GdkPixbufFromSkBitmap(*image.bitmap());
  gtk_status_icon_set_from_pixbuf(icon_, pixbuf);
  g_object_unref(pixbuf);
}

void StatusIcon::SetPressedImage(const gfx::ImageSkia& image) {
  // Ignore pressed images, since the standard on Linux is to not highlight
  // pressed status icons.
}

void StatusIcon::SetToolTip(const std::string& tool_tip) {
  gtk_status_icon_set_tooltip_text(icon_, tool_tip.c_str());
}

void StatusIcon::SetContextMenu(ui::SimpleMenuModel* menu_model) {
  menu_.reset(new MenuGtk(NULL, menu_model));
}

void StatusIcon::OnPopupMenu(GtkWidget* widget, guint button, guint time) {
  // If we have a menu - display it.
  if (menu_.get())
    menu_->PopupAsContextForStatusIcon(time, button, icon_);
}

void StatusIcon::OnActivate(GtkWidget* widget) {
  NotifyClicked();
}

}  // namespace atom
