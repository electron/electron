// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_clipboard.h"

#include <string>

#include "ui/base/clipboard/clipboard.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace api {

// static
v8::Handle<v8::Value> Clipboard::Has(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  std::string format_string(*v8::String::Utf8Value(args[0]));
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  return scope.Close(v8::Boolean::New(
      clipboard->IsFormatAvailable(format, ui::Clipboard::BUFFER_STANDARD)));
}

// static
v8::Handle<v8::Value> Clipboard::Read(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  std::string format_string(*v8::String::Utf8Value(args[0]));
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);

  return scope.Close(v8::String::New(data.c_str(), data.size()));
}

// static
v8::Handle<v8::Value> Clipboard::ReadText(const v8::Arguments &args) {
  v8::HandleScope scope;

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  std::string data;
  clipboard->ReadAsciiText(ui::Clipboard::BUFFER_STANDARD, &data);

  return scope.Close(v8::String::New(data.c_str(), data.size()));
}

// static
v8::Handle<v8::Value> Clipboard::WriteText(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (!args[0]->IsString())
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::string text(*v8::String::Utf8Value(args[0]));

  ui::Clipboard::ObjectMap object_map;
  object_map[ui::Clipboard::CBF_TEXT].push_back(
      std::vector<char>(text.begin(), text.end()));
  clipboard->WriteObjects(ui::Clipboard::BUFFER_STANDARD, object_map, NULL);

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Clipboard::Clear(const v8::Arguments &args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->Clear(ui::Clipboard::BUFFER_STANDARD);

  return v8::Undefined();
}

// static
void Clipboard::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "has", Has);
  node::SetMethod(target, "read", Read);
  node::SetMethod(target, "readText", ReadText);
  node::SetMethod(target, "writeText", WriteText);
  node::SetMethod(target, "clear", Clear);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_clipboard, atom::api::Clipboard::Initialize)
