// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/message_box.h"

#include <gtk/gtk.h>

#include "base/callback.h"
#include "browser/native_window.h"

namespace atom {

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail) {
  return 0;
}

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const MessageBoxCallback& callback) {
  GtkWindow* window = parent_window ? parent_window->GetNativeWindow() : NULL;
  GtkWidget* dialog = gtk_dialog_new_with_buttons(
      title.c_str(),
      window,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
      NULL);

  for (size_t i = 0; i < buttons.size(); ++i)
    gtk_dialog_add_button(GTK_DIALOG(dialog), buttons[i].c_str(), i);

  GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget* message_label = gtk_label_new(message.c_str());
  gtk_box_pack_start(GTK_BOX(content_area), message_label, FALSE, FALSE, 0);
  GtkWidget* detail_label = gtk_label_new(detail.c_str());
  gtk_box_pack_start(GTK_BOX(content_area), detail_label, FALSE, FALSE, 0);

  gtk_widget_show_all(dialog);

  callback.Run(0);
}

}  // namespace atom
