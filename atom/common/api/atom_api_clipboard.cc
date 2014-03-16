// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_clipboard.h"

#include <string>
#include <vector>

#include "atom/common/v8/native_type_conversions.h"
#include "ui/base/clipboard/clipboard.h"

#include "atom/common/v8/node_common.h"

namespace atom {

namespace api {

// static
void Clipboard::Has(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string format_string;
  if (!FromV8Arguments(args, &format_string))
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  args.GetReturnValue().Set(
      clipboard->IsFormatAvailable(format, ui::Clipboard::BUFFER_STANDARD));
}

// static
void Clipboard::Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string format_string;
  if (!FromV8Arguments(args, &format_string))
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);

  args.GetReturnValue().Set(ToV8Value(data));
}

// static
void Clipboard::ReadText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  std::string data;
  clipboard->ReadAsciiText(ui::Clipboard::BUFFER_STANDARD, &data);

  args.GetReturnValue().Set(ToV8Value(data));
}

// static
void Clipboard::WriteText(const v8::FunctionCallbackInfo<v8::Value>& args) {
  std::string text;
  if (!FromV8Arguments(args, &text))
    return node::ThrowTypeError("Bad argument");

  ui::Clipboard::ObjectMap object_map;
  object_map[ui::Clipboard::CBF_TEXT].push_back(
      std::vector<char>(text.begin(), text.end()));

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->WriteObjects(ui::Clipboard::BUFFER_STANDARD, object_map);
}

// static
void Clipboard::Clear(const v8::FunctionCallbackInfo<v8::Value>& args) {
  ui::Clipboard::GetForCurrentThread()->Clear(ui::Clipboard::BUFFER_STANDARD);
}

// static
void Clipboard::Initialize(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "has", Has);
  NODE_SET_METHOD(target, "read", Read);
  NODE_SET_METHOD(target, "readText", ReadText);
  NODE_SET_METHOD(target, "writeText", WriteText);
  NODE_SET_METHOD(target, "clear", Clear);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_clipboard, atom::api::Clipboard::Initialize)
