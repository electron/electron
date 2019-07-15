// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "native_mate/dictionary.h"
#include "shell/browser/api/atom_api_browser_window.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/certificate_trust.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/message_box.h"
#include "shell/common/native_mate_converters/callback.h"
#include "shell/common/native_mate_converters/file_dialog_converter.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/native_mate_converters/image_converter.h"
#include "shell/common/native_mate_converters/message_box_converter.h"
#include "shell/common/native_mate_converters/net_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/promise_util.h"

namespace {

int ShowMessageBoxSync(const electron::MessageBoxSettings& settings) {
  return electron::ShowMessageBoxSync(settings);
}

void ResolvePromiseObject(electron::util::Promise promise,
                          int result,
                          bool checkbox_checked) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(promise.isolate());

  dict.Set("response", result);
  dict.Set("checkboxChecked", checkbox_checked);

  promise.Resolve(dict.GetHandle());
}

v8::Local<v8::Promise> ShowMessageBox(
    const electron::MessageBoxSettings& settings,
    mate::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  electron::util::Promise promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  electron::ShowMessageBox(
      settings, base::BindOnce(&ResolvePromiseObject, std::move(promise)));

  return handle;
}

void ShowOpenDialogSync(const file_dialog::DialogSettings& settings,
                        mate::Arguments* args) {
  std::vector<base::FilePath> paths;
  if (file_dialog::ShowOpenDialogSync(settings, &paths))
    args->Return(paths);
}

v8::Local<v8::Promise> ShowOpenDialog(
    const file_dialog::DialogSettings& settings,
    mate::Arguments* args) {
  electron::util::Promise promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();
  file_dialog::ShowOpenDialog(settings, std::move(promise));
  return handle;
}

void ShowSaveDialogSync(const file_dialog::DialogSettings& settings,
                        mate::Arguments* args) {
  base::FilePath path;
  if (file_dialog::ShowSaveDialogSync(settings, &path))
    args->Return(path);
}

v8::Local<v8::Promise> ShowSaveDialog(
    const file_dialog::DialogSettings& settings,
    mate::Arguments* args) {
  electron::util::Promise promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  file_dialog::ShowSaveDialog(settings, std::move(promise));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("showMessageBoxSync", &ShowMessageBoxSync);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("showErrorBox", &electron::ShowErrorBox);
  dict.SetMethod("showOpenDialogSync", &ShowOpenDialogSync);
  dict.SetMethod("showOpenDialog", &ShowOpenDialog);
  dict.SetMethod("showSaveDialogSync", &ShowSaveDialogSync);
  dict.SetMethod("showSaveDialog", &ShowSaveDialog);
#if defined(OS_MACOSX) || defined(OS_WIN)
  dict.SetMethod("showCertificateTrustDialog",
                 &certificate_trust::ShowCertificateTrust);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_browser_dialog, Initialize)
