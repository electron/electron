// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include <gtk/gtk.h>

#include "atom/browser/browser.h"
#include "base/callback.h"
#include "base/strings/utf_string_conversions.h"

#define ANSI_FOREGROUND_RED   "\x1b[31m"
#define ANSI_FOREGROUND_BLACK "\x1b[30m"
#define ANSI_TEXT_BOLD        "\x1b[1m"
#define ANSI_BACKGROUND_GRAY  "\x1b[47m"
#define ANSI_RESET            "\x1b[0m"

namespace atom {

int ShowMessageBox(NativeWindow* parent_window,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  return 0;
}

void ShowMessageBox(NativeWindow* parent_window,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  callback.Run(0);
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
