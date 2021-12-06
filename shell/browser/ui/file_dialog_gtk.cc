// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <gmodule.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/gtk_util.h"
#include "shell/browser/unresponsive_suppressor.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/gtk/gtk_util.h"

#if defined(USE_X11)
#include "ui/events/platform/x11/x11_event_source.h"
#endif

#if defined(USE_OZONE) || defined(USE_X11)
#include "ui/base/ui_base_features.h"
#endif

namespace file_dialog {

static GModule* gtk_module;
static absl::optional<bool> supports_gtk_file_chooser_native;

using dl_gtk_native_dialog_show_t = void (*)(void*);
using dl_gtk_native_dialog_destroy_t = void (*)(void*);
using dl_gtk_native_dialog_set_modal_t = void (*)(void*, gboolean);
using dl_gtk_native_dialog_run_t = int (*)(void*);
using dl_gtk_native_dialog_hide_t = void (*)(void*);
using dl_gtk_file_chooser_native_new_t = void* (*)(const char*,
                                                   GtkWindow*,
                                                   GtkFileChooserAction,
                                                   const char*,
                                                   const char*);

static dl_gtk_native_dialog_show_t dl_gtk_native_dialog_show;
static dl_gtk_native_dialog_destroy_t dl_gtk_native_dialog_destroy;
static dl_gtk_native_dialog_set_modal_t dl_gtk_native_dialog_set_modal;
static dl_gtk_native_dialog_run_t dl_gtk_native_dialog_run;
static dl_gtk_native_dialog_hide_t dl_gtk_native_dialog_hide;
static dl_gtk_file_chooser_native_new_t dl_gtk_file_chooser_native_new;

DialogSettings::DialogSettings() = default;
DialogSettings::DialogSettings(const DialogSettings&) = default;
DialogSettings::~DialogSettings() = default;

namespace {

static const int kPreviewWidth = 256;
static const int kPreviewHeight = 512;

void InitGtkFileChooserNativeSupport() {
  // Return early if we have already setup the native functions or we have tried
  // once before and failed. Avoid running expensive dynamic library operations.
  if (supports_gtk_file_chooser_native) {
    return;
  }

  // Mark that we have attempted to initialize support at least once
  supports_gtk_file_chooser_native = false;

  if (!g_module_supported()) {
    return;
  }

  gtk_module = g_module_open("libgtk-3.so", G_MODULE_BIND_LAZY);
  if (!gtk_module) {
    return;
  }

  // Will never be unloaded
  g_module_make_resident(gtk_module);

  bool found = g_module_symbol(
      gtk_module, "gtk_file_chooser_native_new",
      reinterpret_cast<void**>(&dl_gtk_file_chooser_native_new));
  if (!found) {
    return;
  }

  found = g_module_symbol(
      gtk_module, "gtk_native_dialog_set_modal",
      reinterpret_cast<void**>(&dl_gtk_native_dialog_set_modal));
  if (!found) {
    return;
  }

  found =
      g_module_symbol(gtk_module, "gtk_native_dialog_destroy",
                      reinterpret_cast<void**>(&dl_gtk_native_dialog_destroy));
  if (!found) {
    return;
  }

  found = g_module_symbol(gtk_module, "gtk_native_dialog_show",
                          reinterpret_cast<void**>(&dl_gtk_native_dialog_show));
  if (!found) {
    return;
  }

  found = g_module_symbol(gtk_module, "gtk_native_dialog_hide",
                          reinterpret_cast<void**>(&dl_gtk_native_dialog_hide));
  if (!found) {
    return;
  }

  found = g_module_symbol(gtk_module, "gtk_native_dialog_run",
                          reinterpret_cast<void**>(&dl_gtk_native_dialog_run));
  if (!found) {
    return;
  }

  supports_gtk_file_chooser_native =
      dl_gtk_file_chooser_native_new && dl_gtk_native_dialog_set_modal &&
      dl_gtk_native_dialog_destroy && dl_gtk_native_dialog_run &&
      dl_gtk_native_dialog_show && dl_gtk_native_dialog_hide;
}

class FileChooserDialog {
 public:
  FileChooserDialog(GtkFileChooserAction action, const DialogSettings& settings)
      : parent_(
            static_cast<electron::NativeWindowViews*>(settings.parent_window)),
        filters_(settings.filters) {
    InitGtkFileChooserNativeSupport();

    auto label = settings.button_label;

    if (*supports_gtk_file_chooser_native) {
      dialog_ = GTK_FILE_CHOOSER(dl_gtk_file_chooser_native_new(
          settings.title.c_str(), NULL, action,
          label.empty() ? nullptr : label.c_str(), nullptr));
    } else {
      const char* confirm_text = gtk_util::GetOkLabel();
      if (!label.empty())
        confirm_text = label.c_str();
      else if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
        confirm_text = gtk_util::GetSaveLabel();
      else if (action == GTK_FILE_CHOOSER_ACTION_OPEN)
        confirm_text = gtk_util::GetOpenLabel();

      dialog_ = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new(
          settings.title.c_str(), NULL, action, gtk_util::GetCancelLabel(),
          GTK_RESPONSE_CANCEL, confirm_text, GTK_RESPONSE_ACCEPT, NULL));
    }

    if (parent_) {
      parent_->SetEnabled(false);
      if (*supports_gtk_file_chooser_native) {
        dl_gtk_native_dialog_set_modal(static_cast<void*>(dialog_), TRUE);
      } else {
        gtk::SetGtkTransientForAura(GTK_WIDGET(dialog_),
                                    parent_->GetNativeWindow());
        gtk_window_set_modal(GTK_WINDOW(dialog_), TRUE);
      }
    }

    if (action == GTK_FILE_CHOOSER_ACTION_SAVE)
      gtk_file_chooser_set_do_overwrite_confirmation(dialog_, TRUE);
    if (action != GTK_FILE_CHOOSER_ACTION_OPEN)
      gtk_file_chooser_set_create_folders(dialog_, TRUE);

    if (!settings.default_path.empty()) {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      if (base::DirectoryExists(settings.default_path)) {
        gtk_file_chooser_set_current_folder(
            dialog_, settings.default_path.value().c_str());
      } else {
        if (settings.default_path.IsAbsolute()) {
          gtk_file_chooser_set_current_folder(
              dialog_, settings.default_path.DirName().value().c_str());
        }

        gtk_file_chooser_set_current_name(
            GTK_FILE_CHOOSER(dialog_),
            settings.default_path.BaseName().value().c_str());
      }
    }

    if (!settings.filters.empty())
      AddFilters(settings.filters);

    // GtkFileChooserNative does not support preview widgets through the
    // org.freedesktop.portal.FileChooser portal. In the case of running through
    // the org.freedesktop.portal.FileChooser portal, anything having to do with
    // the update-preview signal or the preview widget will just be ignored.
    if (!*supports_gtk_file_chooser_native) {
      preview_ = gtk_image_new();
      g_signal_connect(dialog_, "update-preview",
                       G_CALLBACK(OnUpdatePreviewThunk), this);
      gtk_file_chooser_set_preview_widget(dialog_, preview_);
    }
  }

  ~FileChooserDialog() {
    if (*supports_gtk_file_chooser_native) {
      dl_gtk_native_dialog_destroy(static_cast<void*>(dialog_));
    } else {
      gtk_widget_destroy(GTK_WIDGET(dialog_));
    }

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
    auto* file_chooser = dialog();
    gtk_file_chooser_set_show_hidden(file_chooser,
                                     hasProp(SAVE_DIALOG_SHOW_HIDDEN_FILES));
    gtk_file_chooser_set_do_overwrite_confirmation(
        file_chooser, hasProp(SAVE_DIALOG_SHOW_OVERWRITE_CONFIRMATION));
  }

  void RunAsynchronous() {
    g_signal_connect(dialog_, "response", G_CALLBACK(OnFileDialogResponseThunk),
                     this);
    if (*supports_gtk_file_chooser_native) {
      dl_gtk_native_dialog_show(static_cast<void*>(dialog_));
    } else {
      gtk_widget_show_all(GTK_WIDGET(dialog_));

#if defined(USE_X11)
      // We need to call gtk_window_present after making the widgets visible
      // to make sure window gets correctly raised and gets focus.
      x11::Time time = ui::X11EventSource::GetInstance()->GetTimestamp();
      gtk_window_present_with_time(GTK_WINDOW(dialog_),
                                   static_cast<uint32_t>(time));
#endif
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
                     GtkWidget*,
                     int);

  GtkFileChooser* dialog() const { return dialog_; }

 private:
  void AddFilters(const Filters& filters);

  electron::NativeWindowViews* parent_;
  electron::UnresponsiveSuppressor unresponsive_suppressor_;

  GtkFileChooser* dialog_;
  GtkWidget* preview_;

  Filters filters_;
  std::unique_ptr<gin_helper::Promise<gin_helper::Dictionary>> save_promise_;
  std::unique_ptr<gin_helper::Promise<gin_helper::Dictionary>> open_promise_;

  // Callback for when we update the preview for the selection.
  CHROMEG_CALLBACK_0(FileChooserDialog, void, OnUpdatePreview, GtkFileChooser*);

  DISALLOW_COPY_AND_ASSIGN(FileChooserDialog);
};

void FileChooserDialog::OnFileDialogResponse(GtkWidget* widget, int response) {
  if (*supports_gtk_file_chooser_native) {
    dl_gtk_native_dialog_hide(static_cast<void*>(dialog_));
  } else {
    gtk_widget_hide(GTK_WIDGET(dialog_));
  }
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
      // guarantee a pure lowercase variant
      std::string file_extension = base::ToLowerASCII("*." + extension);
      gtk_file_filter_add_pattern(gtk_filter, file_extension.c_str());
      // guarantee a pure uppercase variant
      file_extension = base::ToUpperASCII("*." + extension);
      gtk_file_filter_add_pattern(gtk_filter, file_extension.c_str());
    }

    gtk_file_filter_set_name(gtk_filter, filter.first.c_str());
    gtk_file_chooser_add_filter(dialog_, gtk_filter);
  }
}

bool CanPreview(const struct stat& st) {
  // Only preview regular files; pipes may hang.
  // See https://crbug.com/534754.
  if (!S_ISREG(st.st_mode)) {
    return false;
  }

  // Don't preview huge files; they may crash.
  // https://github.com/electron/electron/issues/31630
  // Setting an arbitrary filesize max t at 100 MB here.
  constexpr off_t ArbitraryMax = 100000000ULL;
  return st.st_size < ArbitraryMax;
}

void FileChooserDialog::OnUpdatePreview(GtkFileChooser* chooser) {
  CHECK(!*supports_gtk_file_chooser_native);
  gchar* filename = gtk_file_chooser_get_preview_filename(chooser);
  if (!filename) {
    gtk_file_chooser_set_preview_widget_active(chooser, FALSE);
    return;
  }

  struct stat sb;
  if (stat(filename, &sb) != 0 || !CanPreview(sb)) {
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
  // gtk_native_dialog_run() will call gtk_native_dialog_show() for us.
  if (!*supports_gtk_file_chooser_native) {
    gtk_widget_show_all(GTK_WIDGET(dialog.dialog()));
  }
}

int RunFileDialog(const FileChooserDialog& dialog) {
  int response = 0;
  if (*supports_gtk_file_chooser_native) {
    response = dl_gtk_native_dialog_run(static_cast<void*>(dialog.dialog()));
  } else {
    response = gtk_dialog_run(GTK_DIALOG(dialog.dialog()));
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
  save_dialog.SetupSaveProperties(settings.properties);

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
