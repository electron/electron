// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/bind.h"
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/ui/message_box.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/function_converter.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace {

void ShowMessageBox(int type,
                    const std::vector<std::string>& buttons,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  atom::MessageBoxCallback callback;
  if (mate::Converter<atom::MessageBoxCallback>::FromV8(args->isolate(),
                                                        peek,
                                                        &callback)) {
    atom::ShowMessageBox(window, (atom::MessageBoxType)type, buttons, title,
                         message, detail, callback);
  } else {
    int chosen = atom::ShowMessageBox(window, (atom::MessageBoxType)type,
                                      buttons, title, message, detail);
    args->Return(chosen);
  }
}

void ShowOpenDialog(const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  file_dialog::OpenDialogCallback callback;
  if (mate::Converter<file_dialog::OpenDialogCallback>::FromV8(args->isolate(),
                                                               peek,
                                                               &callback)) {
    file_dialog::ShowOpenDialog(window, title, default_path, properties,
                                callback);
  } else {
    std::vector<base::FilePath> paths;
    if (file_dialog::ShowOpenDialog(window, title, default_path, properties,
                                    &paths))
      args->Return(paths);
  }
}

void ShowSaveDialog(const std::string& title,
                    const base::FilePath& default_path,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Handle<v8::Value> peek = args->PeekNext();
  file_dialog::SaveDialogCallback callback;
  if (mate::Converter<file_dialog::SaveDialogCallback>::FromV8(args->isolate(),
                                                               peek,
                                                               &callback)) {
    file_dialog::ShowSaveDialog(window, title, default_path, callback);
  } else {
    base::FilePath path;
    if (file_dialog::ShowSaveDialog(window, title, default_path, &path))
      args->Return(path);
  }
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("showOpenDialog", &ShowOpenDialog);
  dict.SetMethod("showSaveDialog", &ShowSaveDialog);
}

}  // namespace

NODE_MODULE_X(atom_browser_dialog, Initialize, NULL, NM_F_BUILTIN)
