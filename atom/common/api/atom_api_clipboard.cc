// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "native_mate/dictionary.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/image/image.h"

#include "atom/common/node_includes.h"

namespace mate {

template<>
struct Converter<ui::ClipboardType> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     ui::ClipboardType* out) {
    std::string type;
    if (!Converter<std::string>::FromV8(isolate, val, &type))
      return false;

    if (type == "selection")
      *out = ui::CLIPBOARD_TYPE_SELECTION;
    else
      *out = ui::CLIPBOARD_TYPE_COPY_PASTE;
    return true;
  }
};

}  // namespace mate

namespace {

bool Has(const std::string& format_string, ui::ClipboardType type) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));
  return clipboard->IsFormatAvailable(format, type);
}

std::string Read(const std::string& format_string,
                 ui::ClipboardType type) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

base::string16 ReadText(ui::ClipboardType type) {
  base::string16 data;
  ui::Clipboard::GetForCurrentThread()->ReadText(type, &data);
  return data;
}

void WriteText(const base::string16& text, ui::ClipboardType type) {
  ui::ScopedClipboardWriter writer(type);
  writer.WriteText(text);
}

gfx::Image ReadImage(ui::ClipboardType type) {
  SkBitmap bitmap = ui::Clipboard::GetForCurrentThread()->ReadImage(type);
  return gfx::Image::CreateFrom1xBitmap(bitmap);
}

void Clear(ui::ClipboardType type) {
  ui::Clipboard::GetForCurrentThread()->Clear(type);
}

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("_has", &Has);
  dict.SetMethod("_read", &Read);
  dict.SetMethod("_readText", &ReadText);
  dict.SetMethod("_writeText", &WriteText);
  dict.SetMethod("_readImage", &ReadImage);
  dict.SetMethod("_clear", &Clear);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_clipboard, Initialize)
