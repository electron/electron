// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/image/image.h"

#include "atom/common/node_includes.h"

namespace {

ui::ClipboardType GetClipboardType(mate::Arguments* args) {
  std::string type;
  if (args->GetNext(&type) && type == "selection")
    return ui::CLIPBOARD_TYPE_SELECTION;
  else
    return ui::CLIPBOARD_TYPE_COPY_PASTE;
}

std::vector<base::string16> AvailableFormats(mate::Arguments* args) {
  std::vector<base::string16> format_types;
  bool ignore;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadAvailableTypes(GetClipboardType(args), &format_types, &ignore);
  return format_types;
}

bool Has(const std::string& format_string, mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));
  return clipboard->IsFormatAvailable(format, GetClipboardType(args));
}

std::string Read(const std::string& format_string,
                 mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

void Write(const mate::Dictionary& data,
           mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  base::string16 text, html;
  gfx::Image image;

  if (data.Get("text", &text))
    writer.WriteText(text);

  if (data.Get("html", &html))
    writer.WriteHTML(html, std::string());

  if (data.Get("image", &image))
    writer.WriteImage(image.AsBitmap());
}

base::string16 ReadText(mate::Arguments* args) {
  base::string16 data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  auto type = GetClipboardType(args);
  if (clipboard->IsFormatAvailable(
      ui::Clipboard::GetPlainTextWFormatType(), type)) {
    clipboard->ReadText(type, &data);
  } else if (clipboard->IsFormatAvailable(
             ui::Clipboard::GetPlainTextFormatType(), type)) {
    std::string result;
    clipboard->ReadAsciiText(type, &result);
    data = base::ASCIIToUTF16(result);
  }
  return data;
}

void WriteText(const base::string16& text, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteText(text);
}

base::string16 ReadHtml(mate::Arguments* args) {
  base::string16 data;
  base::string16 html;
  std::string url;
  uint32 start;
  uint32 end;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadHTML(GetClipboardType(args), &html, &url, &start, &end);
  data = html.substr(start, end - start);
  return data;
}

void WriteHtml(const base::string16& html, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteHTML(html, std::string());
}

gfx::Image ReadImage(mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  SkBitmap bitmap = clipboard->ReadImage(GetClipboardType(args));
  return gfx::Image::CreateFrom1xBitmap(bitmap);
}

void WriteImage(const gfx::Image& image, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteImage(image.AsBitmap());
}

void Clear(mate::Arguments* args) {
  ui::Clipboard::GetForCurrentThread()->Clear(GetClipboardType(args));
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("availableFormats", &AvailableFormats);
  dict.SetMethod("has", &Has);
  dict.SetMethod("read", &Read);
  dict.SetMethod("write", &Write);
  dict.SetMethod("readText", &ReadText);
  dict.SetMethod("writeText", &WriteText);
  dict.SetMethod("readHtml", &ReadHtml);
  dict.SetMethod("writeHtml", &WriteHtml);
  dict.SetMethod("readImage", &ReadImage);
  dict.SetMethod("writeImage", &WriteImage);
  dict.SetMethod("clear", &Clear);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_clipboard, Initialize)
