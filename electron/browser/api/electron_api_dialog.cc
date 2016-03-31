// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "electron/browser/api/electron_api_window.h"
#include "electron/browser/native_window.h"
#include "electron/browser/ui/file_dialog.h"
#include "electron/browser/ui/message_box.h"
#include "electron/common/native_mate_converters/callback.h"
#include "electron/common/native_mate_converters/file_path_converter.h"
#include "electron/common/native_mate_converters/image_converter.h"
#include "native_mate/dictionary.h"

#include "electron/common/node_includes.h"

namespace mate {

template<>
struct Converter<file_dialog::Filter> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::Filter* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("name", &(out->first)))
      return false;
    if (!dict.Get("extensions", &(out->second)))
      return false;
    return true;
  }
};

}  // namespace mate

namespace {

void ShowMessageBox(int type,
                    const std::vector<std::string>& buttons,
                    int default_id,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    electron::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Local<v8::Value> peek = args->PeekNext();
  electron::MessageBoxCallback callback;
  if (mate::Converter<electron::MessageBoxCallback>::FromV8(args->isolate(),
                                                        peek,
                                                        &callback)) {
    electron::ShowMessageBox(window, (electron::MessageBoxType)type, buttons,
                         default_id, cancel_id, options, title,
                         message, detail, icon, callback);
  } else {
    int chosen = electron::ShowMessageBox(window, (electron::MessageBoxType)type,
                                      buttons, default_id, cancel_id,
                                      options, title, message, detail, icon);
    args->Return(chosen);
  }
}

void ShowOpenDialog(const std::string& title,
                    const base::FilePath& default_path,
                    const file_dialog::Filters& filters,
                    int properties,
                    electron::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Local<v8::Value> peek = args->PeekNext();
  file_dialog::OpenDialogCallback callback;
  if (mate::Converter<file_dialog::OpenDialogCallback>::FromV8(args->isolate(),
                                                               peek,
                                                               &callback)) {
    file_dialog::ShowOpenDialog(window, title, default_path, filters,
                                properties, callback);
  } else {
    std::vector<base::FilePath> paths;
    if (file_dialog::ShowOpenDialog(window, title, default_path, filters,
                                    properties, &paths))
      args->Return(paths);
  }
}

void ShowSaveDialog(const std::string& title,
                    const base::FilePath& default_path,
                    const file_dialog::Filters& filters,
                    electron::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Local<v8::Value> peek = args->PeekNext();
  file_dialog::SaveDialogCallback callback;
  if (mate::Converter<file_dialog::SaveDialogCallback>::FromV8(args->isolate(),
                                                               peek,
                                                               &callback)) {
    file_dialog::ShowSaveDialog(window, title, default_path, filters, callback);
  } else {
    base::FilePath path;
    if (file_dialog::ShowSaveDialog(window, title, default_path, filters,
                                    &path))
      args->Return(path);
  }
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("showErrorBox", &electron::ShowErrorBox);
  dict.SetMethod("showOpenDialog", &ShowOpenDialog);
  dict.SetMethod("showSaveDialog", &ShowSaveDialog);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(electron_browser_dialog, Initialize)
