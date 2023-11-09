// Copyright (c) 2023 Microsoft, Inc.
// Copyright (c) 2011 The Chromium Authors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/status_icon_gtk.h"

#include <gtk/gtk.h>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/ui/gtk/menu_gtk.h"
#include "shell/browser/ui/gtk_util.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

StatusIconGtk::StatusIconGtk() : icon_(TakeGObject(gtk_status_icon_new())) {
  auto connect = [&](auto* sender, const char* detailed_signal, auto receiver) {
    // Unretained() is safe since StatusIconGtk will own the
    // ScopedGSignal.
    signals_.emplace_back(
        sender, detailed_signal,
        base::BindRepeating(receiver, base::Unretained(this)));
  };
  connect(icon_.get(), "activate", &StatusIconGtk::OnClick);
  connect(icon_.get(), "popup_menu", &StatusIconGtk::OnContextMenuRequested);
}

StatusIconGtk::~StatusIconGtk() = default;

void StatusIconGtk::SetIcon(const gfx::ImageSkia& image) {
  if (image.isNull())
    return;

  GdkPixbuf* pixbuf = gtk_util::GdkPixbufFromSkBitmap(*image.bitmap());
  gtk_status_icon_set_from_pixbuf(icon_, pixbuf);
  g_object_unref(pixbuf);
}

void StatusIconGtk::SetToolTip(const std::u16string& tool_tip) {
  gtk_status_icon_set_tooltip_text(icon_, base::UTF16ToUTF8(tool_tip).c_str());
}

void StatusIconGtk::UpdatePlatformContextMenu(ui::MenuModel* model) {
  if (model)
    menu_ = std::make_unique<gtkui::MenuGtk>(model);
}

void StatusIconGtk::RefreshPlatformContextMenu() {
  if (menu_)
    menu_->Refresh();
}

void StatusIconGtk::OnSetDelegate() {
  SetIcon(delegate_->GetImage());
  SetToolTip(delegate_->GetToolTip());
  UpdatePlatformContextMenu(delegate_->GetMenuModel());
  gtk_status_icon_set_visible(icon_, TRUE);
}

void StatusIconGtk::OnClick(GtkStatusIcon* status_icon) {
  delegate_->OnClick();
}

void StatusIconGtk::OnContextMenuRequested(GtkStatusIcon* status_icon,
                                           guint button,
                                           guint32 activate_time) {
  if (menu_.get()) {
    gtk_menu_popup(menu_->GetGtkMenu(), nullptr, nullptr,
                   gtk_status_icon_position_menu, icon_, button, activate_time);
  }
}

}  // namespace electron
