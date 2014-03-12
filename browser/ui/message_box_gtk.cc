// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/message_box.h"

#include "base/callback.h"
#include "browser/native_window.h"
#include "browser/ui/gtk/gtk_util.h"
#include "ui/base/gtk/gtk_signal.h"

namespace atom {

namespace {

class MessageBox {
 public:
  MessageBox(NativeWindow* parent_window,
             MessageBoxType type,
             const std::vector<std::string>& buttons,
             const std::string& title,
             const std::string& message,
             const std::string& detail) {
    GtkWindow* window = parent_window ? parent_window->GetNativeWindow() : NULL;
    dialog_ = gtk_dialog_new_with_buttons(
        title.c_str(),
        window,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
        NULL);

    for (size_t i = 0; i < buttons.size(); ++i)
      gtk_dialog_add_button(GTK_DIALOG(dialog_), buttons[i].c_str(), i);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog_));
    GtkWidget* message_label = gtk_util::CreateBoldLabel(message);
    gtk_util::LeftAlignMisc(message_label);
    gtk_box_pack_start(GTK_BOX(content_area), message_label, FALSE, FALSE, 0);
    GtkWidget* detail_label = gtk_label_new(detail.c_str());
    gtk_util::LeftAlignMisc(detail_label);
    gtk_box_pack_start(GTK_BOX(content_area), detail_label, FALSE, FALSE, 0);

    gtk_window_set_resizable(GTK_WINDOW(dialog_), FALSE);
  }

  ~MessageBox() {
    gtk_widget_destroy(dialog_);
  }

  void RunAsynchronous(const MessageBoxCallback& callback) {
    callback_ = callback;
    g_signal_connect(dialog_, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(dialog_, "response",
                     G_CALLBACK(OnResponseDialogThunk), this);
    gtk_widget_show_all(dialog_);
  }

  CHROMEGTK_CALLBACK_1(MessageBox, void, OnResponseDialog, int);

  GtkWidget* dialog() const { return dialog_; }

 private:
  GtkWidget* dialog_;
  MessageBoxCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(MessageBox);
};

void MessageBox::OnResponseDialog(GtkWidget* widget, int response) {
  gtk_widget_hide_all(dialog_);
  callback_.Run(response);
  delete this;
}

}  // namespace

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
  MessageBox* message_box = new MessageBox(
      parent_window, type, buttons, title, message, detail);
  message_box->RunAsynchronous(callback);
}

}  // namespace atom
