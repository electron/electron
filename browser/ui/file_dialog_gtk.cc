// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/file_dialog.h"

#include "base/callback.h"
#include "browser/native_window.h"
#include "browser/ui/gtk/gtk_util.h"
#include "ui/base/gtk/gtk_signal.h"

namespace file_dialog {

namespace {

class FileChooserDialog {
 public:
  FileChooserDialog(GtkFileChooserAction action,
                    atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path) {
    GtkWindow* window = parent_window ? parent_window->GetNativeWindow() : NULL;
    dialog_ = gtk_file_chooser_dialog_new(
        title.c_str(),
        window,
        action,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
  }

  virtual ~FileChooserDialog() {
    gtk_widget_destroy(dialog_);
  }

  void RunSaveAsynchronous(const SaveDialogCallback& callback) {
    save_callback_ = callback;
    g_signal_connect(dialog_, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(dialog_, "response",
                     G_CALLBACK(OnSelectSingleFileDialogResponseThunk), this);
    gtk_widget_show_all(dialog_);
  }

  CHROMEGTK_CALLBACK_1(FileChooserDialog, void,
                       OnSelectSingleFileDialogResponse, int);

 private:
  GtkWidget* dialog_;

  SaveDialogCallback save_callback_;
  OpenDialogCallback open_callback_;

  DISALLOW_COPY_AND_ASSIGN(FileChooserDialog);
};

void FileChooserDialog::OnSelectSingleFileDialogResponse(GtkWidget* widget,
                                                         int response) {
  gtk_widget_hide_all(dialog_);

  if (response == GTK_RESPONSE_ACCEPT) {
    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_));
    save_callback_.Run(true, base::FilePath(filename));
  } else {
    save_callback_.Run(false, base::FilePath());
  }

  delete this;
}

}  // namespace

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  return false;
}

void ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    const OpenDialogCallback& callback) {
  callback.Run(false, std::vector<base::FilePath>());
}

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path) {
  return false;
}

void ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const SaveDialogCallback& callback) {
  FileChooserDialog* dialog = new FileChooserDialog(
      GTK_FILE_CHOOSER_ACTION_SAVE, parent_window, title, default_path);
  dialog->RunSaveAsynchronous(callback);
}

}  // namespace file_dialog
