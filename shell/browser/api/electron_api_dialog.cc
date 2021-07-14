// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "shell/browser/ui/certificate_trust.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/message_box_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace {

int ShowMessageBoxSync(const electron::MessageBoxSettings& settings) {
  return electron::ShowMessageBoxSync(settings);
}

void ResolvePromiseObject(gin_helper::Promise<gin_helper::Dictionary> promise,
                          int result,
                          bool checkbox_checked) {
  v8::Isolate* isolate = promise.isolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("response", result);
  dict.Set("checkboxChecked", checkbox_checked);

  promise.Resolve(dict);
}

v8::Local<v8::Promise> ShowMessageBox(
    const electron::MessageBoxSettings& settings,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  electron::ShowMessageBox(
      settings, base::BindOnce(&ResolvePromiseObject, std::move(promise)));

  return handle;
}

void ShowOpenDialogSync(const file_dialog::DialogSettings& settings,
                        gin::Arguments* args) {
  std::vector<base::FilePath> paths;
  if (file_dialog::ShowOpenDialogSync(settings, &paths))
    args->Return(paths);
}

v8::Local<v8::Promise> ShowOpenDialog(
    const file_dialog::DialogSettings& settings,
    gin::Arguments* args) {
  gin_helper::Promise<gin_helper::Dictionary> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();
  file_dialog::ShowOpenDialog(settings, std::move(promise));
  return handle;
}

void ShowSaveDialogSync(const file_dialog::DialogSettings& settings,
                        gin::Arguments* args) {
  base::FilePath path;
  if (file_dialog::ShowSaveDialogSync(settings, &path))
    args->Return(path);
}

v8::Local<v8::Promise> ShowSaveDialog(
    const file_dialog::DialogSettings& settings,
    gin::Arguments* args) {
  gin_helper::Promise<gin_helper::Dictionary> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  file_dialog::ShowSaveDialog(settings, std::move(promise));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("showMessageBoxSync", &ShowMessageBoxSync);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("_closeMessageBox", &electron::CloseMessageBox);
  dict.SetMethod("showErrorBox", &electron::ShowErrorBox);
  dict.SetMethod("showOpenDialogSync", &ShowOpenDialogSync);
  dict.SetMethod("showOpenDialog", &ShowOpenDialog);
  dict.SetMethod("showSaveDialogSync", &ShowSaveDialogSync);
  dict.SetMethod("showSaveDialog", &ShowSaveDialog);
#if defined(OS_MAC) || defined(OS_WIN)
  dict.SetMethod("showCertificateTrustDialog",
                 &certificate_trust::ShowCertificateTrust);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_dialog, Initialize)
