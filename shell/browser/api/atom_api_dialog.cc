// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "shell/browser/api/atom_api_browser_window.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/certificate_trust.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/gin_converters/file_dialog_converter_gin_adapter.h"
#include "shell/common/gin_converters/message_box_converter.h"
#include "shell/common/gin_converters/net_converter_gin_adapter.h"
// #include "shell/common/native_mate_converters/callback.h"
// #include "shell/common/native_mate_converters/file_path_converter.h"
// #include "shell/common/native_mate_converters/image_converter.h"
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

  auto showMessageBoxSyncT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowMessageBoxSync));
  dict.Set("showMessageBoxSync",
           showMessageBoxSyncT->GetFunction(context).ToLocalChecked());

  auto showMessageBoxT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowMessageBox));
  dict.Set("showMessageBox",
           showMessageBoxT->GetFunction(context).ToLocalChecked());

  auto showErrorBoxT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&electron::ShowErrorBox));
  dict.Set("showErrorBox",
           showErrorBoxT->GetFunction(context).ToLocalChecked());

  auto showOpenDialogSyncT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowOpenDialogSync));
  dict.Set("showOpenDialogSync",
           showOpenDialogSyncT->GetFunction(context).ToLocalChecked());

  auto showOpenDialogT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowOpenDialog));
  dict.Set("showOpenDialog",
           showOpenDialogT->GetFunction(context).ToLocalChecked());

  auto showSaveDialogSyncT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowSaveDialogSync));
  dict.Set("showSaveDialogSync",
           showSaveDialogSyncT->GetFunction(context).ToLocalChecked());

  auto showSaveDialogT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&ShowSaveDialog));
  dict.Set("showSaveDialog",
           showSaveDialogT->GetFunction(context).ToLocalChecked());

#if defined(OS_MACOSX) || defined(OS_WIN)
  auto showCertificateTrustDialogT = gin::CreateFunctionTemplate(
      isolate, base::BindRepeating(&certificate_trust::ShowCertificateTrust));
  dict.Set("showCertificateTrustDialog",
           showCertificateTrustDialogT->GetFunction(context).ToLocalChecked());
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_dialog, Initialize)
