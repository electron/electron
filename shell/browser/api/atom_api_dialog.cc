// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "gin/dictionary.h"
#include "shell/browser/ui/certificate_trust.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/api/gin_utils.h"
#include "shell/common/gin_converters/file_dialog_converter.h"
#include "shell/common/gin_converters/message_box_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/promise_util.h"

namespace {

int ShowMessageBoxSync(const electron::MessageBoxSettings& settings) {
  return electron::ShowMessageBoxSync(settings);
}

void ResolvePromiseObject(electron::util::Promise promise,
                          int result,
                          bool checkbox_checked) {
  v8::Isolate* isolate = promise.isolate();
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("response", result);
  dict.Set("checkboxChecked", checkbox_checked);

  promise.Resolve(gin::ConvertToV8(isolate, dict));
}

v8::Local<v8::Promise> ShowMessageBox(
    const electron::MessageBoxSettings& settings,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  electron::util::Promise promise(isolate);
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
  electron::util::Promise promise(args->isolate());
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
  electron::util::Promise promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  file_dialog::ShowSaveDialog(settings, std::move(promise));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin::Dictionary dict(isolate, exports);
  dict.Set("showMessageBoxSync",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowMessageBoxSync)));
  dict.Set("showMessageBox",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowMessageBox)));
  dict.Set("showErrorBox",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&electron::ShowErrorBox)));
  dict.Set("showOpenDialogSync",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowOpenDialogSync)));
  dict.Set("showOpenDialog",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowOpenDialog)));
  dict.Set("showSaveDialogSync",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowSaveDialogSync)));
  dict.Set("showSaveDialog",
           gin::ConvertCallbackToV8Leaked(
               isolate, base::BindRepeating(&ShowSaveDialog)));
#if defined(OS_MACOSX) || defined(OS_WIN)
  dict.Set("showCertificateTrustDialog",
           gin::ConvertCallbackToV8Leaked(
               isolate,
               base::BindRepeating(&certificate_trust::ShowCertificateTrust)));
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_dialog, Initialize)
