// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_signal.h"
#include "chrome/browser/ui/libgtk2ui/gtk2_util.h"
#include "chrome/browser/ui/libgtk2ui/skia_utils_gtk2.h"

#define ANSI_FOREGROUND_RED   "\x1b[31m"
#define ANSI_FOREGROUND_BLACK "\x1b[30m"
#define ANSI_TEXT_BOLD        "\x1b[1m"
#define ANSI_BACKGROUND_GRAY  "\x1b[47m"
#define ANSI_RESET            "\x1b[0m"

namespace atom {

namespace {

class GtkMessageBox {
 public:
  GtkMessageBox(bool synchronous,
                NativeWindow* parent_window,
                MessageBoxType type,
                const std::vector<std::string>& buttons,
                const std::string& title,
                const std::string& message,
                const std::string& detail,
                const gfx::ImageSkia& icon)
      : dialog_scope_(parent_window),
        cancel_id_(0) {
    // The dialog should be modal when showed synchronous.
    int flags = GTK_DIALOG_NO_SEPARATOR;
    if (synchronous)
      flags |= GTK_DIALOG_MODAL;

    // Create dialog.
    dialog_ = gtk_message_dialog_new(
        nullptr,  // parent
        static_cast<GtkDialogFlags>(flags),  // flags
        GTK_MESSAGE_OTHER,  // no icon
        GTK_BUTTONS_NONE,  // no buttons
        "%s", message.c_str());
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog_),
        "%s", detail.empty() ? nullptr : detail.c_str());

    // Set dialog's icon.
    if (!icon.isNull()) {
      GdkPixbuf* pixbuf = libgtk2ui::GdkPixbufFromSkBitmap(*icon.bitmap());
      GtkWidget* image = gtk_image_new_from_pixbuf(pixbuf);
      gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog_), image);
      gtk_widget_show(image);
      g_object_unref(pixbuf);
    }

    // Add buttons.
    for (size_t i = 0; i < buttons.size(); ++i) {
      gtk_dialog_add_button(GTK_DIALOG(dialog_),
                            TranslateToStock(i, buttons[i]),
                            i);
    }

    // Parent window.
    if (parent_window) {
      gfx::NativeWindow window = parent_window->GetNativeWindow();
      libgtk2ui::SetGtkTransientForAura(dialog_, window);
    }
  }

  ~GtkMessageBox() {
    gtk_widget_destroy(dialog_);
  }

  const char* TranslateToStock(int id, const std::string& text) {
    std::string lower = base::StringToLowerASCII(text);
    if (lower == "cancel") {
      cancel_id_ = id;
      return GTK_STOCK_CANCEL;
    } else if (lower == "no") {
      cancel_id_ = id;
      return GTK_STOCK_NO;
    } else if (lower == "ok") {
      return GTK_STOCK_OK;
    } else if (lower == "yes") {
      return GTK_STOCK_YES;
    } else {
      return text.c_str();
    }
  }

  void RunAsynchronous(const MessageBoxCallback& callback) {
    callback_ = callback;
    g_signal_connect(dialog_, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), nullptr);
    g_signal_connect(dialog_, "response",
                     G_CALLBACK(OnResponseDialogThunk), this);
    gtk_widget_show_all(dialog_);
  }

  CHROMEGTK_CALLBACK_1(GtkMessageBox, void, OnResponseDialog, int);

  GtkWidget* dialog() const { return dialog_; }
  int cancel_id() const { return cancel_id_; }

 private:
  atom::NativeWindow::DialogScope dialog_scope_;

  // The id to return when the dialog is closed without pressing buttons.
  int cancel_id_;

  GtkWidget* dialog_;
  MessageBoxCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(GtkMessageBox);
};

void GtkMessageBox::OnResponseDialog(GtkWidget* widget, int response) {
  gtk_widget_hide_all(dialog_);

  if (response < 0)
    callback_.Run(cancel_id_);
  else
    callback_.Run(response);
  delete this;
}

}  // namespace

int ShowMessageBox(NativeWindow* parent,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  GtkMessageBox box(true, parent, type, buttons, title, message, detail, icon);
  gtk_widget_show_all(box.dialog());
  int response = gtk_dialog_run(GTK_DIALOG(box.dialog()));
  if (response < 0)
    return box.cancel_id();
  else
    return response;
}

void ShowMessageBox(NativeWindow* parent,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  auto box = new GtkMessageBox(
      false, parent, type, buttons, title, message, detail, icon);
  box->RunAsynchronous(callback);
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
  if (Browser::Get()->is_ready()) {
    GtkWidget* dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "%s", base::UTF16ToUTF8(title).c_str());
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog),
        "%s", base::UTF16ToUTF8(content).c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  } else {
    fprintf(stderr,
            ANSI_TEXT_BOLD ANSI_BACKGROUND_GRAY
            ANSI_FOREGROUND_RED  "%s\n"
            ANSI_FOREGROUND_BLACK "%s"
            ANSI_RESET "\n",
            base::UTF16ToUTF8(title).c_str(),
            base::UTF16ToUTF8(content).c_str());
  }
}

}  // namespace atom
