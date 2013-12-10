// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_dialog.h"

#include "base/bind.h"
#include "browser/native_window.h"
#include "browser/ui/file_dialog.h"
#include "browser/ui/message_box.h"
#include "common/v8_conversions.h"
#include "vendor/node/src/node_internals.h"

using node::node_isolate;

namespace atom {

namespace api {

namespace {

template<typename T>
void CallV8Function(v8::Persistent<v8::Function> callback, T arg) {
  DCHECK(!callback.IsEmpty());

  v8::HandleScope scope;
  v8::Handle<v8::Value> value = ToV8Value(arg);
  callback->Call(callback, 1, &value);
  callback.Dispose(node_isolate);
}

template<typename T>
void CallV8Function2(v8::Persistent<v8::Function> callback,
                     bool result,
                     T arg) {
  if (result)
    return CallV8Function<T>(callback, arg);
  else
    return CallV8Function<void*>(callback, NULL);
}

void Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  NODE_SET_METHOD(target, "showMessageBox", ShowMessageBox);
  NODE_SET_METHOD(target, "showOpenDialog", ShowOpenDialog);
  NODE_SET_METHOD(target, "showSaveDialog", ShowSaveDialog);
}

}  // namespace

v8::Handle<v8::Value> ShowMessageBox(const v8::Arguments& args) {
  v8::HandleScope scope;

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
    return scope.Close(v8::Integer::New(chosen));
  } else {
    v8::Persistent<v8::Function> callback = FromV8Value(args[6]);
    atom::ShowMessageBox(
        native_window,
        (MessageBoxType)type,
        buttons,
        title,
        message,
        detail,
        base::Bind(&CallV8Function<int>, callback));
    return v8::Undefined();
  }
}

v8::Handle<v8::Value> ShowOpenDialog(const v8::Arguments& args) {
  v8::HandleScope scope;

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
      return v8::Undefined();

    v8::Handle<v8::Array> result = v8::Array::New(paths.size());
    for (size_t i = 0; i < paths.size(); ++i)
      result->Set(i, ToV8Value(paths[i]));

    return scope.Close(result);
  } else {
    v8::Persistent<v8::Function> callback = FromV8Value(args[4]);
    file_dialog::ShowOpenDialog(
        native_window,
        title,
        default_path,
        properties,
        base::Bind(&CallV8Function2<const std::vector<base::FilePath>&>,
                   callback));
    return v8::Undefined();
  }
}

v8::Handle<v8::Value> ShowSaveDialog(const v8::Arguments& args) {
  v8::HandleScope scope;

  std::string title;
  base::FilePath default_path;
  if (!FromV8Arguments(args, &title, &default_path))
    return node::ThrowTypeError("Bad argument");

  NativeWindow* native_window = FromV8Value(args[2]);

  if (!args[3]->IsFunction()) {
    base::FilePath path;
    if (!file_dialog::ShowSaveDialog(native_window,
                                     title,
                                     default_path,
                                     &path))
      return v8::Undefined();

    return scope.Close(ToV8Value(path));
  } else {
    v8::Persistent<v8::Function> callback = FromV8Value(args[3]);
    file_dialog::ShowSaveDialog(
        native_window,
        title,
        default_path,
        base::Bind(&CallV8Function2<const base::FilePath&>, callback));
    return v8::Undefined();
  }
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_dialog, atom::api::Initialize)
