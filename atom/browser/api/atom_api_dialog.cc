// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/atom_api_browser_window.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/certificate_trust.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/ui/message_box.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_dialog_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/promise_util.h"
#include "native_mate/dictionary.h"

namespace {

int ShowMessageBoxSync(int type,
                       const std::vector<std::string>& buttons,
                       int default_id,
                       int cancel_id,
                       int options,
                       const std::string& title,
                       const std::string& message,
                       const std::string& detail,
                       const std::string& checkbox_label,
                       bool checkbox_checked,
                       const gfx::ImageSkia& icon,
                       atom::NativeWindow* window) {
  return atom::ShowMessageBoxSync(
      window, static_cast<atom::MessageBoxType>(type), buttons, default_id,
      cancel_id, options, title, message, detail, icon);
}

void ResolvePromiseObject(atom::util::Promise promise,
                          int result,
                          bool checkbox_checked) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(promise.isolate());

  dict.Set("response", result);
  dict.Set("checkboxChecked", checkbox_checked);

  promise.Resolve(dict.GetHandle());
}

v8::Local<v8::Promise> ShowMessageBox(int type,
                                      const std::vector<std::string>& buttons,
                                      int default_id,
                                      int cancel_id,
                                      int options,
                                      const std::string& title,
                                      const std::string& message,
                                      const std::string& detail,
                                      const std::string& checkbox_label,
                                      bool checkbox_checked,
                                      const gfx::ImageSkia& icon,
                                      atom::NativeWindow* window,
                                      mate::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  atom::util::Promise promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  atom::ShowMessageBox(
      window, static_cast<atom::MessageBoxType>(type), buttons, default_id,
      cancel_id, options, title, message, detail, checkbox_label,
      checkbox_checked, icon,
      base::BindOnce(&ResolvePromiseObject, std::move(promise)));

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
  atom::util::Promise promise(args->isolate());
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
  atom::util::Promise promise(args->isolate());
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
  dict.SetMethod("showErrorBox", &atom::ShowErrorBox);
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
