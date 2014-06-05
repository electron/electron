// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/dictionary.h"
#include "ui/base/clipboard/clipboard.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<ui::Clipboard::Buffer> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     ui::Clipboard::Buffer* out) {
    std::string type;
    if (!Converter<std::string>::FromV8(isolate, val, &type))
      return false;

    if (type == "selection")
      *out = ui::Clipboard::BUFFER_SELECTION;
    else
      *out = ui::Clipboard::BUFFER_STANDARD;
    return true;
  }
};

}  // namespace mate

namespace {

bool Has(const std::string& format_string, ui::Clipboard::Buffer buffer) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));
  return clipboard->IsFormatAvailable(format, buffer);
}

std::string Read(const std::string& format_string,
                 ui::Clipboard::Buffer buffer) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

string16 ReadText(ui::Clipboard::Buffer buffer) {
  string16 data;
  ui::Clipboard::GetForCurrentThread()->ReadText(buffer, &data);
  return data;
}

void WriteText(const std::string text, ui::Clipboard::Buffer buffer) {
  ui::Clipboard::ObjectMap object_map;
  object_map[ui::Clipboard::CBF_TEXT].push_back(
      std::vector<char>(text.begin(), text.end()));

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->WriteObjects(buffer, object_map);
}

void Clear(ui::Clipboard::Buffer buffer) {
  ui::Clipboard::GetForCurrentThread()->Clear(buffer);
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("_has", &Has);
  dict.SetMethod("_read", &Read);
  dict.SetMethod("_readText", &ReadText);
  dict.SetMethod("_writeText", &WriteText);
  dict.SetMethod("_clear", &Clear);
}

}  // namespace

NODE_MODULE(atom_common_clipboard, Initialize)
