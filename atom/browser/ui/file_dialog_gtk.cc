// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/file_dialog.h"

#include "atom/browser/native_window.h"
#include "base/callback.h"
#include "base/file_util.h"
#include "chrome/browser/ui/gtk/gtk_util.h"
#include "ui/base/gtk/gtk_signal.h"

namespace file_dialog {

namespace {

class FileChooserDialog {
 public:
  FileChooserDialog(GtkFileChooserAction action,
                    atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path)
      : dialog_scope_(new atom::NativeWindow::DialogScope(parent_window)) {
    const char* confirm_text = GTK_STOCK_OK;
    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      confirm_text = GTK_STOCK_SAVE;
    else if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
      confirm_text = GTK_STOCK_OPEN;

    GtkWindow* window = parent_window ? parent_window->GetNativeWindow() : NULL;
    dialog_ = gtk_file_chooser_dialog_new(
        title.c_str(),
        window,
        action,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        confirm_text, GTK_RESPONSE_ACCEPT,
        NULL);

    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog_),
                                                     TRUE);
    if (action != GTK_FILE_CHOOSER_ACTION_OPEN)
      gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog_), TRUE);

    // Set window-to-parent modality by adding the dialog to the same window
    // group as the parent.
    gtk_window_group_add_window(gtk_window_get_group(window),
                                GTK_WINDOW(dialog_));
    gtk_window_set_modal(GTK_WINDOW(dialog_), TRUE);

    if (!default_path.empty()) {
      if (base::DirectoryExists(default_path))
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog_),
                                            default_path.value().c_str());
      else
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog_),
                                      default_path.value().c_str());
    }
  }

  virtual ~FileChooserDialog() {
    gtk_widget_destroy(dialog_);
  }

  void RunAsynchronous() {
    g_signal_connect(dialog_, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(dialog_, "response",
                     G_CALLBACK(OnFileDialogResponseThunk), this);
    gtk_widget_show_all(dialog_);
  }

  void RunSaveAsynchronous(const SaveDialogCallback& callback) {
    save_callback_ = callback;
    RunAsynchronous();
  }

  void RunOpenAsynchronous(const OpenDialogCallback& callback) {
    open_callback_ = callback;
    RunAsynchronous();
  }

  base::FilePath GetFileName() const {
    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_));
    base::FilePath path(filename);
    g_free(filename);
    return path;
  }

  std::vector<base::FilePath> GetFileNames() const {
    std::vector<base::FilePath> paths;
    GSList* filenames = gtk_file_chooser_get_filenames(
        GTK_FILE_CHOOSER(dialog_));
    for (GSList* iter = filenames; iter != NULL; iter = g_slist_next(iter)) {
      base::FilePath path(static_cast<char*>(iter->data));
      g_free(iter->data);
      paths.push_back(path);
    }
    g_slist_free(filenames);
    return paths;
  }

  CHROMEGTK_CALLBACK_1(FileChooserDialog, void, OnFileDialogResponse, int);

  GtkWidget* dialog() const { return dialog_; }

 private:
  GtkWidget* dialog_;

  SaveDialogCallback save_callback_;
  OpenDialogCallback open_callback_;

  scoped_ptr<atom::NativeWindow::DialogScope> dialog_scope_;

  DISALLOW_COPY_AND_ASSIGN(FileChooserDialog);
};

void FileChooserDialog::OnFileDialogResponse(GtkWidget* widget, int response) {
  gtk_widget_hide_all(dialog_);

  if (!save_callback_.is_null()) {
    if (response == GTK_RESPONSE_ACCEPT)
      save_callback_.Run(true, GetFileName());
    else
      save_callback_.Run(false, base::FilePath());
  } else if (!open_callback_.is_null()) {
    if (response == GTK_RESPONSE_ACCEPT)
      open_callback_.Run(true, GetFileNames());
    else
      open_callback_.Run(false, std::vector<base::FilePath>());
  }
  delete this;
}

}  // namespace

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  if (properties & FILE_DIALOG_OPEN_DIRECTORY)
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  FileChooserDialog open_dialog(action, parent_window, title, default_path);
  if (properties & FILE_DIALOG_MULTI_SELECTIONS)
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(open_dialog.dialog()),
                                         TRUE);

  gtk_widget_show_all(open_dialog.dialog());
  int response = gtk_dialog_run(GTK_DIALOG(open_dialog.dialog()));
  if (response == GTK_RESPONSE_ACCEPT) {
    *paths = open_dialog.GetFileNames();
    return true;
  } else {
    return false;
  }
}

void ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    const OpenDialogCallback& callback) {
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  if (properties & FILE_DIALOG_OPEN_DIRECTORY)
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  FileChooserDialog* open_dialog = new FileChooserDialog(
      action, parent_window, title, default_path);
  if (properties & FILE_DIALOG_MULTI_SELECTIONS)
    gtk_file_chooser_set_select_multiple(
        GTK_FILE_CHOOSER(open_dialog->dialog()), TRUE);

  open_dialog->RunOpenAsynchronous(callback);
}

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path) {
  FileChooserDialog save_dialog(
      GTK_FILE_CHOOSER_ACTION_SAVE, parent_window, title, default_path);
  gtk_widget_show_all(save_dialog.dialog());
  int response = gtk_dialog_run(GTK_DIALOG(save_dialog.dialog()));
  if (response == GTK_RESPONSE_ACCEPT) {
    *path = save_dialog.GetFileName();
    return true;
  } else {
    return false;
  }
}

void ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const SaveDialogCallback& callback) {
  FileChooserDialog* save_dialog = new FileChooserDialog(
      GTK_FILE_CHOOSER_ACTION_SAVE, parent_window, title, default_path);
  save_dialog->RunSaveAsynchronous(callback);
}

}  // namespace file_dialog
