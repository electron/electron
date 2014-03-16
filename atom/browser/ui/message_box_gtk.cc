// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include "atom/browser/native_window.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "chrome/browser/ui/gtk/gtk_util.h"
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
             const std::string& detail)
      : cancel_id_(0) {
    GtkWindow* window = parent_window ? parent_window->GetNativeWindow() : NULL;
    dialog_ = gtk_dialog_new_with_buttons(
        title.c_str(),
        window,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
        NULL);

    for (size_t i = 0; i < buttons.size(); ++i)
      gtk_dialog_add_button(GTK_DIALOG(dialog_),
                            TranslateToStock(i, buttons[i]),
                            i);

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

  const char* TranslateToStock(int id, const std::string& text) {
    if (LowerCaseEqualsASCII(text, "cancel")) {
      cancel_id_ = id;
      return GTK_STOCK_CANCEL;
    } else if (LowerCaseEqualsASCII(text, "no")) {
      cancel_id_ = id;
      return GTK_STOCK_NO;
    } else if (LowerCaseEqualsASCII(text, "ok")) {
      return GTK_STOCK_OK;
    } else if (LowerCaseEqualsASCII(text, "yes")) {
      return GTK_STOCK_YES;
    } else {
      return text.c_str();
    }
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
  int cancel_id() const { return cancel_id_; }

 private:
  GtkWidget* dialog_;
  MessageBoxCallback callback_;

  // The id to return when the dialog is closed without pressing buttons.
  int cancel_id_;

  DISALLOW_COPY_AND_ASSIGN(MessageBox);
};

void MessageBox::OnResponseDialog(GtkWidget* widget, int response) {
  gtk_widget_hide_all(dialog_);

  if (response < 0)
    callback_.Run(cancel_id_);
  else
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
  MessageBox message_box(parent_window, type, buttons, title, message, detail);
  gtk_widget_show_all(message_box.dialog());
  int response = gtk_dialog_run(GTK_DIALOG(message_box.dialog()));
  if (response < 0)
    return message_box.cancel_id();
  else
    return response;
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
