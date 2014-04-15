// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "native_mate/object_template_builder.h"
#include "ui/base/clipboard/clipboard.h"

#include "atom/common/v8/node_common.h"

namespace {

bool Has(const std::string& format_string) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));
  return clipboard->IsFormatAvailable(format, ui::Clipboard::BUFFER_STANDARD);
}

std::string Read(const std::string& format_string) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

std::string ReadText() {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  std::string data;
  clipboard->ReadAsciiText(ui::Clipboard::BUFFER_STANDARD, &data);
  return data;
}

void WriteText(const std::string text) {
  ui::Clipboard::ObjectMap object_map;
  object_map[ui::Clipboard::CBF_TEXT].push_back(
      std::vector<char>(text.begin(), text.end()));

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->WriteObjects(ui::Clipboard::BUFFER_STANDARD, object_map);
}

void Clear() {
  ui::Clipboard::GetForCurrentThread()->Clear(ui::Clipboard::BUFFER_STANDARD);
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::ObjectTemplateBuilder builder(v8::Isolate::GetCurrent());
  builder.SetMethod("has", &Has)
         .SetMethod("read", &Read)
         .SetMethod("readText", &ReadText)
         .SetMethod("writeText", &WriteText)
         .SetMethod("clear", &Clear);
  exports->SetPrototype(builder.Build()->NewInstance());
}

}  // namespace

NODE_MODULE(atom_common_clipboard, Initialize)
