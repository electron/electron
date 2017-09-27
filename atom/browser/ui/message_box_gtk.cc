// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include "atom/browser/browser.h"
#include "atom/browser/native_window_observer.h"
#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/libgtkui/gtk_signal.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "chrome/browser/ui/libgtkui/skia_utils_gtk.h"
#include "ui/views/widget/desktop_aura/x11_desktop_handler.h"

#define ANSI_FOREGROUND_RED   "\x1b[31m"
#define ANSI_FOREGROUND_BLACK "\x1b[30m"
#define ANSI_TEXT_BOLD        "\x1b[1m"
#define ANSI_BACKGROUND_GRAY  "\x1b[47m"
#define ANSI_RESET            "\x1b[0m"

namespace atom {

namespace {

class GtkMessageBox : public NativeWindowObserver {
 public:
  GtkMessageBox(NativeWindow* parent_window,
                MessageBoxType type,
                const std::vector<std::string>& buttons,
                int default_id,
                int cancel_id,
                const std::string& title,
                const std::string& message,
                const std::string& detail,
                const std::string& checkbox_label,
                bool checkbox_checked,
                const gfx::ImageSkia& icon)
      : cancel_id_(cancel_id),
        checkbox_checked_(false),
        parent_(static_cast<NativeWindow*>(parent_window)) {
    // Create dialog.
    dialog_ = gtk_message_dialog_new(
        nullptr,  // parent
        static_cast<GtkDialogFlags>(0),  // no flags
        GetMessageType(type),  // type
        GTK_BUTTONS_NONE,  // no buttons
        "%s", message.c_str());
    if (!detail.empty())
      gtk_message_dialog_format_secondary_text(
          GTK_MESSAGE_DIALOG(dialog_), "%s", detail.c_str());
    if (!title.empty())
      gtk_window_set_title(GTK_WINDOW(dialog_), title.c_str());

    // Set dialog's icon.
    if (!icon.isNull()) {
      GdkPixbuf* pixbuf = libgtkui::GdkPixbufFromSkBitmap(*icon.bitmap());
      GtkIconSource* iconsource = gtk_icon_source_new();
      GtkIconSet* iconset = gtk_icon_set_new();
      gtk_icon_source_set_pixbuf(iconsource, pixbuf);
      gtk_icon_set_add_source(iconset, iconsource);
      GtkWidget* image = gtk_image_new_from_icon_set(iconset,
                                                     GTK_ICON_SIZE_DIALOG);
      gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog_), image);
      gtk_widget_show(image);
      gtk_icon_source_free(iconsource);
      gtk_icon_set_unref(iconset);
      g_object_unref(pixbuf);
    }

    if (!checkbox_label.empty()) {
      GtkWidget* message_area =
          gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG(dialog_));
      GtkWidget* check_button =
          gtk_check_button_new_with_label(checkbox_label.c_str());
      g_signal_connect(check_button, "toggled",
                       G_CALLBACK(OnCheckboxToggledThunk), this);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
                                   checkbox_checked);
      gtk_container_add(GTK_CONTAINER(message_area), check_button);
    }

    // Add buttons.
    for (size_t i = 0; i < buttons.size(); ++i) {
      GtkWidget* button = gtk_dialog_add_button(
          GTK_DIALOG(dialog_), TranslateToStock(i, buttons[i]), i);
      if (static_cast<int>(i) == default_id)
        gtk_widget_grab_focus(button);
    }

    // Parent window.
    if (parent_) {
      parent_->AddObserver(this);
      static_cast<NativeWindowViews*>(parent_)->SetEnabled(false);
      libgtkui::SetGtkTransientForAura(dialog_, parent_->GetNativeWindow());
      gtk_window_set_modal(GTK_WINDOW(dialog_), TRUE);
    }
  }

  ~GtkMessageBox() {
    gtk_widget_destroy(dialog_);
    if (parent_) {
      parent_->RemoveObserver(this);
      static_cast<NativeWindowViews*>(parent_)->SetEnabled(true);
    }
  }

  GtkMessageType GetMessageType(MessageBoxType type) {
    switch (type) {
      case MESSAGE_BOX_TYPE_INFORMATION:
        return GTK_MESSAGE_INFO;
      case MESSAGE_BOX_TYPE_WARNING:
        return GTK_MESSAGE_WARNING;
      case MESSAGE_BOX_TYPE_QUESTION:
        return GTK_MESSAGE_QUESTION;
      case MESSAGE_BOX_TYPE_ERROR:
        return GTK_MESSAGE_ERROR;
      default:
        return GTK_MESSAGE_OTHER;
    }
  }

  const char* TranslateToStock(int id, const std::string& text) {
    std::string lower = base::ToLowerASCII(text);
    if (lower == "cancel")
      return GTK_STOCK_CANCEL;
    else if (lower == "no")
      return GTK_STOCK_NO;
    else if (lower == "ok")
      return GTK_STOCK_OK;
    else if (lower == "yes")
      return GTK_STOCK_YES;
    else
      return text.c_str();
  }

  void Show() {
    gtk_widget_show_all(dialog_);
    // We need to call gtk_window_present after making the widgets visible to
    // make sure window gets correctly raised and gets focus.
    int time = ui::X11EventSource::GetInstance()->GetTimestamp();
    gtk_window_present_with_time(GTK_WINDOW(dialog_), time);
  }

  int RunSynchronous() {
    Show();
    int response = gtk_dialog_run(GTK_DIALOG(dialog_));
    if (response < 0)
      return cancel_id_;
    else
      return response;
  }

  void RunAsynchronous(const MessageBoxCallback& callback) {
    callback_ = callback;
    g_signal_connect(dialog_, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), nullptr);
    g_signal_connect(dialog_, "response",
                     G_CALLBACK(OnResponseDialogThunk), this);
    Show();
  }

  void OnWindowClosed() override {
    parent_->RemoveObserver(this);
    parent_ = nullptr;
  }

  CHROMEGTK_CALLBACK_1(GtkMessageBox, void, OnResponseDialog, int);
  CHROMEGTK_CALLBACK_0(GtkMessageBox, void, OnCheckboxToggled);

 private:
  atom::UnresponsiveSuppressor unresponsive_suppressor_;

  // The id to return when the dialog is closed without pressing buttons.
  int cancel_id_;

  bool checkbox_checked_;

  NativeWindow* parent_;
  GtkWidget* dialog_;
  MessageBoxCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(GtkMessageBox);
};

void GtkMessageBox::OnResponseDialog(GtkWidget* widget, int response) {
  gtk_widget_hide(dialog_);

  if (response < 0)
    callback_.Run(cancel_id_, checkbox_checked_);
  else
    callback_.Run(response, checkbox_checked_);
  delete this;
}

void GtkMessageBox::OnCheckboxToggled(GtkWidget* widget) {
  checkbox_checked_ = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}

}  // namespace

int ShowMessageBox(NativeWindow* parent,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   int default_id,
                   int cancel_id,
                   int options,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  return GtkMessageBox(parent, type, buttons, default_id, cancel_id, title,
                       message, detail, "", false, icon)
      .RunSynchronous();
}

void ShowMessageBox(NativeWindow* parent,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    int default_id,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const std::string& checkbox_label,
                    bool checkbox_checked,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  (new GtkMessageBox(parent, type, buttons, default_id, cancel_id, title,
                     message, detail, checkbox_label, checkbox_checked, icon))
      ->RunAsynchronous(callback);
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
  if (Browser::Get()->is_ready()) {
    GtkMessageBox(nullptr, MESSAGE_BOX_TYPE_ERROR, {"OK"}, -1, 0, "Error",
                  base::UTF16ToUTF8(title).c_str(),
                  base::UTF16ToUTF8(content).c_str(), "", false,
                  gfx::ImageSkia())
        .RunSynchronous();
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
