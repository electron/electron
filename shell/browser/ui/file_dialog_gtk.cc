// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <glib/gi18n.h>  // _() macro
#include <gmodule.h>

#include <functional>
#include <memory>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/gtk_util.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "ui/aura/window.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/gtk/gtk_util.h"

#if defined(USE_X11)
#include "ui/events/platform/x11/x11_event_source.h"
#endif

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

namespace file_dialog {

DialogSettings::DialogSettings() = default;
DialogSettings::DialogSettings(const DialogSettings&) = default;
DialogSettings::~DialogSettings() = default;

namespace {

static const int kPreviewWidth = 256;
static const int kPreviewHeight = 512;

// Makes sure that .jpg also shows .JPG.
gboolean FileFilterCaseInsensitive(const GtkFileFilterInfo* file_info,
                                   std::string* file_extension) {
  // Makes .* file extension matches all file types.
  if (*file_extension == ".*")
    return true;
  return base::EndsWith(file_info->filename, *file_extension,
                        base::CompareCase::INSENSITIVE_ASCII);
}

// Deletes |data| when gtk_file_filter_add_custom() is done with it.
void OnFileFilterDataDestroyed(std::string* file_extension) {
  delete file_extension;
}

class FileChooserDialog {
 public:
  FileChooserDialog(GtkFileChooserAction action, const DialogSettings& settings)
      : parent_(
            static_cast<electron::NativeWindowViews*>(settings.parent_window)),
        filters_(settings.filters) {
    const char* confirm_text = gtk_util::kOkLabel;

    if (!settings.button_label.empty())
      confirm_text = settings.button_label.c_str();
    else if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      confirm_text = gtk_util::kSaveLabel;
    else if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
      confirm_text = gtk_util::kOpenLabel;

    gtk_module_ = g_module_open("libgtk-3.so", G_MODULE_BIND_LAZY);
    void* (*dl_gtk_file_chooser_native_new)(const char*, GtkWindow*,
                                            GtkFileChooserAction, const char*,
                                            const char*) = NULL;
    bool found = g_module_symbol(
        gtk_module_, "gtk_file_chooser_native_new",
        reinterpret_cast<void**>(&dl_gtk_file_chooser_native_new));
    if (found && dl_gtk_file_chooser_native_new != NULL) {
      dialog_ = GTK_FILE_CHOOSER(
          dl_gtk_file_chooser_native_new(settings.title.c_str(), NULL, action,
                                         confirm_text, gtk_util::kCancelLabel));
    } else {
      dialog_ = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
          settings.title.c_str(), NULL, action, gtk_util::kCancelLabel,
          GTK_RESPONSE_CANCEL, confirm_text, GTK_RESPONSE_ACCEPT, NULL));
    }

    if (parent_) {
      parent_->SetEnabled(false);
      if (GTK_IS_DIALOG(dialog_))
        gtk::SetGtkTransientForAura(GTK_WIDGET(dialog_),
                                    parent_->GetNativeWindow());
      else
        SetGtkTransientForAura(parent_->GetNativeWindow());
      if (GTK_IS_DIALOG(dialog_)) {
        gtk_window_set_modal(GTK_WINDOW(dialog_), TRUE);
      } else {
        void (*dl_gtk_native_dialog_set_modal)(void*, bool) = NULL;
        g_module_symbol(
            gtk_module_, "gtk_native_dialog_set_modal",
            reinterpret_cast<void**>(&dl_gtk_native_dialog_set_modal));
        dl_gtk_native_dialog_set_modal(static_cast<void*>(dialog_), TRUE);
      }
    }

    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      gtk_file_chooser_set_do_overwrite_confirmation(dialog_, TRUE);
    if (action != GTK_FILE_CHOOSER_ACTION_OPEN)
      gtk_file_chooser_set_create_folders(dialog_, TRUE);

    if (!settings.default_path.empty()) {
      if (base::DirectoryExists(settings.default_path)) {
        gtk_file_chooser_set_current_folder(
            dialog_, settings.default_path.value().c_str());
      } else {
        if (settings.default_path.IsAbsolute()) {
          gtk_file_chooser_set_current_folder(
              dialog_, settings.default_path.DirName().value().c_str());
        }

        gtk_file_chooser_set_current_name(
            dialog_, settings.default_path.BaseName().value().c_str());
      }
    }

    if (!settings.filters.empty())
      AddFilters(settings.filters);

    // GtkNativeDialog does not support preview widgets
    if (GTK_IS_DIALOG(dialog_)) {
      preview_ = gtk_image_new();
      g_signal_connect(dialog_, "update-preview",
                      G_CALLBACK(OnUpdatePreviewThunk), this);
      gtk_file_chooser_set_preview_widget(dialog_, preview_);
    }
  }

  ~FileChooserDialog() {
    if (GTK_IS_DIALOG(dialog_)) {
      gtk_widget_destroy(GTK_WIDGET(dialog_));
    } else {
      void (*dl_gtk_native_dialog_destroy)(void*) = NULL;
      g_module_symbol(gtk_module_, "gtk_native_dialog_destroy",
                      reinterpret_cast<void**>(&dl_gtk_native_dialog_destroy));
      dl_gtk_native_dialog_destroy(static_cast<void*>(dialog_));
    }

    g_clear_pointer(&gtk_module_, g_module_close);

    if (parent_)
      parent_->SetEnabled(true);
  }

  void SetupOpenProperties(int properties) {
    const auto hasProp = [properties](OpenFileDialogProperty prop) {
      return gboolean((properties & prop) != 0);
    };
    auto* file_chooser = dialog();
    gtk_file_chooser_set_select_multiple(file_chooser,
                                         hasProp(OPEN_DIALOG_MULTI_SELECTIONS));
    gtk_file_chooser_set_show_hidden(file_chooser,
                                     hasProp(OPEN_DIALOG_SHOW_HIDDEN_FILES));
  }

  void SetupSaveProperties(int properties) {
    const auto hasProp = [properties](SaveFileDialogProperty prop) {
      return gboolean((properties & prop) != 0);
    };
    auto* file_chooser = GTK_FILE_CHOOSER(dialog());
    gtk_file_chooser_set_show_hidden(file_chooser,
                                     hasProp(SAVE_DIALOG_SHOW_HIDDEN_FILES));
    gtk_file_chooser_set_do_overwrite_confirmation(
        file_chooser, hasProp(SAVE_DIALOG_SHOW_OVERWRITE_CONFIRMATION));
  }

  void RunAsynchronous() {
    if (GTK_IS_DIALOG(dialog_)) {
      g_signal_connect(dialog_, "delete-event",
                       G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    }
    g_signal_connect(dialog_, "response", G_CALLBACK(OnFileDialogResponseThunk),
                     this);
    if (GTK_IS_DIALOG(dialog_)) {
      gtk_widget_show_all(GTK_WIDGET(dialog_));

      // We need to call gtk_window_present after making the widgets visible to
      // make sure window gets correctly raised and gets focus.
      int time = ui::X11EventSource::GetInstance()->GetTimestamp();
      gtk_window_present_with_time(GTK_WINDOW(dialog_), time);
    } else {
      void (*dl_gtk_native_dialog_show)(void*) = NULL;
      g_module_symbol(gtk_module_, "gtk_native_dialog_show",
                      reinterpret_cast<void**>(&dl_gtk_native_dialog_show));
      dl_gtk_native_dialog_show(static_cast<void*>(dialog_));
    }
  }

  void RunSaveAsynchronous(
      gin_helper::Promise<gin_helper::Dictionary> promise) {
    save_promise_ =
        std::make_unique<gin_helper::Promise<gin_helper::Dictionary>>(
            std::move(promise));
    RunAsynchronous();
  }

  void RunOpenAsynchronous(
      gin_helper::Promise<gin_helper::Dictionary> promise) {
    open_promise_ =
        std::make_unique<gin_helper::Promise<gin_helper::Dictionary>>(
            std::move(promise));
    RunAsynchronous();
  }

  base::FilePath GetFileName() const {
    gchar* filename = gtk_file_chooser_get_filename(dialog_);
    const base::FilePath path(filename);
    g_free(filename);
    return path;
  }

  std::vector<base::FilePath> GetFileNames() const {
    std::vector<base::FilePath> paths;
    auto* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog_));
    for (auto* iter = filenames; iter != nullptr; iter = iter->next) {
      auto* filename = static_cast<char*>(iter->data);
      paths.emplace_back(filename);
      g_free(filename);
    }
    g_slist_free(filenames);
    return paths;
  }

  CHROMEG_CALLBACK_1(FileChooserDialog,
                     void,
                     OnFileDialogResponse,
                     GtkFileChooser*,
                     int);

  GtkFileChooser* dialog() const { return dialog_; }
  GModule* gtk_module() const { return gtk_module_; }

 private:
  void AddFilters(const Filters& filters);

  electron::NativeWindowViews* parent_;
  electron::UnresponsiveSuppressor unresponsive_suppressor_;

  GtkFileChooser* dialog_;
  GtkWidget* preview_;
  GModule* gtk_module_;

  Filters filters_;
  std::unique_ptr<gin_helper::Promise<gin_helper::Dictionary>> save_promise_;
  std::unique_ptr<gin_helper::Promise<gin_helper::Dictionary>> open_promise_;

  // Sets |dialog| as transient for |parent|, which will keep it on top and
  // center it above |parent|. Do nothing if |parent| is nullptr.
  void SetGtkTransientForAura(aura::Window* parent) {
    if (!parent || !parent->GetHost())
      return;

    void (*dl_gtk_native_dialog_set_transient_for)(void*, GtkWindow*) = NULL;
    g_module_symbol(
        gtk_module_, "gtk_native_dialog_set_transient_for",
        reinterpret_cast<void**>(&dl_gtk_native_dialog_set_transient_for));
    dl_gtk_native_dialog_set_transient_for(
        dialog_, reinterpret_cast<GtkWindow*>(parent));
  }

  // Callback for when we update the preview for the selection.
  CHROMEG_CALLBACK_0(FileChooserDialog, void, OnUpdatePreview, GtkFileChooser*);

  DISALLOW_COPY_AND_ASSIGN(FileChooserDialog);
};

void FileChooserDialog::OnFileDialogResponse(GtkWidget* widget, int response) {
  gtk_widget_hide(dialog_);
  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  if (save_promise_) {
    gin_helper::Dictionary dict =
        gin::Dictionary::CreateEmpty(save_promise_->isolate());
    if (response == GTK_RESPONSE_ACCEPT) {
      dict.Set("canceled", false);
      dict.Set("filePath", GetFileName());
    } else {
      dict.Set("canceled", true);
      dict.Set("filePath", base::FilePath());
    }
    save_promise_->Resolve(dict);
  } else if (open_promise_) {
    gin_helper::Dictionary dict =
        gin::Dictionary::CreateEmpty(open_promise_->isolate());
    if (response == GTK_RESPONSE_ACCEPT) {
      dict.Set("canceled", false);
      dict.Set("filePaths", GetFileNames());
    } else {
      dict.Set("canceled", true);
      dict.Set("filePaths", std::vector<base::FilePath>());
    }
    open_promise_->Resolve(dict);
  }
  delete this;
}

void FileChooserDialog::AddFilters(const Filters& filters) {
  for (const auto& filter : filters) {
    GtkFileFilter* gtk_filter = gtk_file_filter_new();

    for (const auto& extension : filter.second) {
      auto file_extension = std::make_unique<std::string>("." + extension);
      gtk_file_filter_add_custom(
          gtk_filter, GTK_FILE_FILTER_FILENAME,
          reinterpret_cast<GtkFileFilterFunc>(FileFilterCaseInsensitive),
          file_extension.release(),
          reinterpret_cast<GDestroyNotify>(OnFileFilterDataDestroyed));
    }

    gtk_file_filter_set_name(gtk_filter, filter.first.c_str());
    gtk_file_chooser_add_filter(dialog_, gtk_filter);
  }
}

void FileChooserDialog::OnUpdatePreview(GtkFileChooser* chooser) {
  gchar* filename = gtk_file_chooser_get_preview_filename(chooser);
  if (!filename) {
    gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
    return;
  }

  // Don't attempt to open anything which isn't a regular file. If a named pipe,
  // this may hang. See https://crbug.com/534754.
  struct stat stat_buf;
  if (stat(filename, &stat_buf) != 0 || !S_ISREG(stat_buf.st_mode)) {
    g_free(filename);
    gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
    return;
  }

  // This will preserve the image's aspect ratio.
  GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_size(filename, kPreviewWidth,
                                                       kPreviewHeight, nullptr);
  g_free(filename);
  if (pixbuf) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(preview_), pixbuf);
    g_object_unref(pixbuf);
  }
  gtk_file_chooser_set_preview_widget_active(chooser, pixbuf ? TRUE : FALSE);
}

}  // namespace

void ShowFileDialog(const FileChooserDialog& dialog) {
  if (GTK_IS_DIALOG(dialog.dialog())) {
    gtk_widget_show_all(GTK_WIDGET(dialog.dialog()));
  } else {
    void (*dl_gtk_native_dialog_show)(void*) = NULL;
    g_module_symbol(dialog.gtk_module(), "gtk_native_dialog_show",
                    reinterpret_cast<void**>(&dl_gtk_native_dialog_show));
    dl_gtk_native_dialog_show(static_cast<void*>(dialog.dialog()));
  }
}

int RunFileDialog(const FileChooserDialog& dialog) {
  int response = 0;
  if (GTK_IS_DIALOG(dialog.dialog())) {
    response = gtk_dialog_run(GTK_DIALOG(dialog.dialog()));
  } else {
    int (*dl_gtk_native_dialog_run)(void*) = NULL;
    g_module_symbol(dialog.gtk_module(), "gtk_native_dialog_run",
                    reinterpret_cast<void**>(&dl_gtk_native_dialog_run));
    response = dl_gtk_native_dialog_run(static_cast<void*>(dialog.dialog()));
  }

  return response;
}

bool ShowOpenDialogSync(const DialogSettings& settings,
                        std::vector<base::FilePath>* paths) {
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  if (settings.properties & OPEN_DIALOG_OPEN_DIRECTORY)
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  FileChooserDialog open_dialog(action, settings);
  open_dialog.SetupOpenProperties(settings.properties);

  ShowFileDialog(open_dialog);

  const int response = RunFileDialog(open_dialog);
  if (response == GTK_RESPONSE_ACCEPT) {
    *paths = open_dialog.GetFileNames();
    return true;
  }
  return false;
}

void ShowOpenDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  if (settings.properties & OPEN_DIALOG_OPEN_DIRECTORY)
    action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  FileChooserDialog* open_dialog = new FileChooserDialog(action, settings);
  open_dialog->SetupOpenProperties(settings.properties);
  open_dialog->RunOpenAsynchronous(std::move(promise));
}

bool ShowSaveDialogSync(const DialogSettings& settings, base::FilePath* path) {
  FileChooserDialog save_dialog(GTK_FILE_CHOOSER_ACTION_SAVE, settings);

  ShowFileDialog(save_dialog);

  const int response = RunFileDialog(save_dialog);
  if (response == GTK_RESPONSE_ACCEPT) {
    *path = save_dialog.GetFileName();
    return true;
  }
  return false;
}

void ShowSaveDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise) {
  FileChooserDialog* save_dialog =
      new FileChooserDialog(GTK_FILE_CHOOSER_ACTION_SAVE, settings);
  save_dialog->RunSaveAsynchronous(std::move(promise));
}

}  // namespace file_dialog
