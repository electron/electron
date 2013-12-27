// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_dialog.h"

#include "base/bind.h"
#include "browser/native_window.h"
#include "browser/ui/file_dialog.h"
#include "browser/ui/message_box.h"
#include "common/v8/node_common.h"
#include "common/v8/native_type_conversions.h"

namespace atom {

namespace api {

namespace {

template<typename T>
void CallV8Function(const RefCountedV8Function& callback, T arg) {
  v8::Handle<v8::Value> value = ToV8Value(arg);
  callback->NewHandle(node_isolate)->Call(
      v8::Context::GetCurrent()->Global(), 1, &value);
}

template<typename T>
void CallV8Function2(const RefCountedV8Function& callback, bool result, T arg) {
  if (result)
    return CallV8Function<T>(callback, arg);
  else
    return CallV8Function<void*>(callback, NULL);
}

void Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

  NODE_SET_METHOD(target, "showMessageBox", ShowMessageBox);
  NODE_SET_METHOD(target, "showOpenDialog", ShowOpenDialog);
  NODE_SET_METHOD(target, "showSaveDialog", ShowSaveDialog);
}

}  // namespace

void ShowMessageBox(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int type;
  std::vector<std::string> buttons;
  std::string title, message, detail;
  if (!FromV8Arguments(args, &type, &buttons, &title, &message, &detail))
    return node::ThrowTypeError("Bad argument");

  NativeWindow* native_window = FromV8Value(args[5]);

  if (!args[6]->IsFunction()) {
    int chosen = atom::ShowMessageBox(
        native_window,
        (MessageBoxType)type,
        buttons,
        title,
        message,
        detail);
    args.GetReturnValue().Set(chosen);
  } else {
    RefCountedV8Function callback = FromV8Value(args[6]);
    atom::ShowMessageBox(
        native_window,
        (MessageBoxType)type,
        buttons,
        title,
        message,
        detail,
        base::Bind(&CallV8Function<int>, callback));
  }
}

void ShowOpenDialog(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string title;
  base::FilePath default_path;
  int properties;
  if (!FromV8Arguments(args, &title, &default_path, &properties))
    return node::ThrowTypeError("Bad argument");

  NativeWindow* native_window = FromV8Value(args[3]);

  if (!args[4]->IsFunction()) {
    std::vector<base::FilePath> paths;
    if (!file_dialog::ShowOpenDialog(native_window,
                                     title,
                                     default_path,
                                     properties,
                                     &paths))
      return;

    v8::Handle<v8::Array> result = v8::Array::New(paths.size());
    for (size_t i = 0; i < paths.size(); ++i)
      result->Set(i, ToV8Value(paths[i]));

    args.GetReturnValue().Set(result);
  } else {
    RefCountedV8Function callback = FromV8Value(args[4]);
    file_dialog::ShowOpenDialog(
        native_window,
        title,
        default_path,
        properties,
        base::Bind(&CallV8Function2<const std::vector<base::FilePath>&>,
                   callback));
  }
}

void ShowSaveDialog(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string title;
  base::FilePath default_path;
  if (!FromV8Arguments(args, &title, &default_path))
    return node::ThrowTypeError("Bad argument");

  NativeWindow* native_window = FromV8Value(args[2]);

  if (!args[3]->IsFunction()) {
    base::FilePath path;
    if (file_dialog::ShowSaveDialog(native_window,
                                    title,
                                    default_path,
                                    &path))
      args.GetReturnValue().Set(ToV8Value(path));
  } else {
    RefCountedV8Function callback = FromV8Value(args[3]);
    file_dialog::ShowSaveDialog(
        native_window,
        title,
        default_path,
        base::Bind(&CallV8Function2<const base::FilePath&>, callback));
  }
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_dialog, atom::api::Initialize)
