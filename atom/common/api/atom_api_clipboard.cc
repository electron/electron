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
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
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

std::vector<base::string16> AvailableFormats(ui::ClipboardType type) {
  std::vector<base::string16> format_types;
  bool ignore;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadAvailableTypes(type, &format_types, &ignore);
  return format_types;
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

base::string16 ReadHtml(ui::ClipboardType type) {
  base::string16 data;
  base::string16 html;
  std::string url;
  uint32 start;
  uint32 end;
  ui::Clipboard::GetForCurrentThread()->ReadHTML(type, &html, &url,
                                                 &start, &end);
  data = html.substr(start, end - start);
  return data;
}

void WriteHtml(const base::string16& html,
               ui::ClipboardType type) {
  ui::ScopedClipboardWriter writer(type);
  writer.WriteHTML(html, std::string());
}

gfx::Image ReadImage(ui::ClipboardType type) {
  SkBitmap bitmap = ui::Clipboard::GetForCurrentThread()->ReadImage(type);
  return gfx::Image::CreateFrom1xBitmap(bitmap);
}

void WriteImage(const gfx::Image& image, ui::ClipboardType type) {
  ui::ScopedClipboardWriter writer(type);
  writer.WriteImage(image.AsBitmap());
}

void Clear(ui::ClipboardType type) {
  ui::Clipboard::GetForCurrentThread()->Clear(type);
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("_availableFormats", &AvailableFormats);
  dict.SetMethod("_read", &Read);
  dict.SetMethod("_readText", &ReadText);
  dict.SetMethod("_writeText", &WriteText);
  dict.SetMethod("_readHtml", &ReadHtml);
  dict.SetMethod("_writeHtml", &WriteHtml);
  dict.SetMethod("_readImage", &ReadImage);
  dict.SetMethod("_writeImage", &WriteImage);
  dict.SetMethod("_clear", &Clear);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_clipboard, Initialize)
