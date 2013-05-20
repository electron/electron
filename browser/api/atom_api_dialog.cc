// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_dialog.h"

#include <string>

#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "browser/api/atom_api_window.h"
#include "browser/file_dialog.h"
#include "browser/message_box.h"
#include "browser/native_window.h"

namespace atom {

namespace api {

namespace {

base::FilePath V8ValueToFilePath(v8::Handle<v8::Value> path) {
  std::string path_string(*v8::String::Utf8Value(path));
  return base::FilePath::FromUTF8Unsafe(path_string);
}

v8::Handle<v8::Value> FilePathToV8Value(const base::FilePath path) {
  std::string path_string(path.AsUTF8Unsafe());
  return v8::String::New(path_string.data(), path_string.size());
}

void Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  NODE_SET_METHOD(target, "showMessageBox", ShowMessageBox);
  NODE_SET_METHOD(target, "showOpenDialog", ShowOpenDialog);
  NODE_SET_METHOD(target, "showSaveDialog", ShowSaveDialog);
}

}  // namespace

v8::Handle<v8::Value> ShowMessageBox(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsNumber() ||  // type
      !args[1]->IsArray() ||   // buttons
      !args[2]->IsString() ||  // title
      !args[3]->IsString() ||  // message
      !args[4]->IsString())    // detail
    return node::ThrowTypeError("Bad argument");

  MessageBoxType type = (MessageBoxType)(args[0]->IntegerValue());

  std::vector<std::string> buttons;
  v8::Handle<v8::Array> v8_buttons = v8::Handle<v8::Array>::Cast(args[1]);
  for (uint32_t i = 0; i < v8_buttons->Length(); ++i)
    buttons.push_back(*v8::String::Utf8Value(v8_buttons->Get(i)));

  std::string title(*v8::String::Utf8Value(args[2]));
  std::string message(*v8::String::Utf8Value(args[3]));
  std::string detail(*v8::String::Utf8Value(args[4]));

  int chosen = atom::ShowMessageBox(type, buttons, title, message, detail);
  return scope.Close(v8::Integer::New(chosen));
}

v8::Handle<v8::Value> ShowOpenDialog(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString() ||  // title
      !args[1]->IsString() ||  // default_path
      !args[2]->IsNumber())    // properties
    return node::ThrowTypeError("Bad argument");

  std::string title(*v8::String::Utf8Value(args[0]));
  base::FilePath default_path(V8ValueToFilePath(args[1]));
  int properties = args[2]->IntegerValue();

  std::vector<base::FilePath> paths;
  if (!file_dialog::ShowOpenDialog(title, default_path, properties, &paths))
    return v8::Undefined();

  v8::Handle<v8::Array> result = v8::Array::New(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
    result->Set(i, FilePathToV8Value(paths[i]));

  return scope.Close(result);
}

v8::Handle<v8::Value> ShowSaveDialog(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsObject() ||  // window
      !args[1]->IsString() ||  // title
      !args[2]->IsString())    // default_path
    return node::ThrowTypeError("Bad argument");

  Window* window = Window::Unwrap<Window>(args[0]->ToObject());
  if (!window || !window->window())
    return node::ThrowError("Invalid window");

  std::string title(*v8::String::Utf8Value(args[1]));
  base::FilePath default_path(V8ValueToFilePath(args[2]));

  base::FilePath path;
  if (!file_dialog::ShowSaveDialog(window->window(),
                                   title,
                                   default_path,
                                   &path))
    return v8::Undefined();

  return scope.Close(FilePathToV8Value(path));
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_dialog, atom::api::Initialize)
