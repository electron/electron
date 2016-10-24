// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_clipboard.h"

#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

ui::ClipboardType Clipboard::GetClipboardType(mate::Arguments* args) {
  std::string type;
  if (args->GetNext(&type) && type == "selection")
    return ui::CLIPBOARD_TYPE_SELECTION;
  else
    return ui::CLIPBOARD_TYPE_COPY_PASTE;
}

std::vector<base::string16> Clipboard::AvailableFormats(mate::Arguments* args) {
  std::vector<base::string16> format_types;
  bool ignore;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadAvailableTypes(GetClipboardType(args), &format_types, &ignore);
  return format_types;
}

bool Clipboard::Has(const std::string& format_string, mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));
  return clipboard->IsFormatAvailable(format, GetClipboardType(args));
}

std::string Clipboard::Read(const std::string& format_string,
                            mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::Clipboard::FormatType format(ui::Clipboard::GetFormatType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

void Clipboard::Write(const mate::Dictionary& data, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  base::string16 text, html, bookmark;
  gfx::Image image;

  if (data.Get("text", &text)) {
    writer.WriteText(text);

    if (data.Get("bookmark", &bookmark))
      writer.WriteBookmark(bookmark, base::UTF16ToUTF8(text));
  }

  if (data.Get("rtf", &text)) {
    std::string rtf = base::UTF16ToUTF8(text);
    writer.WriteRTF(rtf);
  }

  if (data.Get("html", &html))
    writer.WriteHTML(html, std::string());

  if (data.Get("image", &image))
    writer.WriteImage(image.AsBitmap());
}

base::string16 Clipboard::ReadText(mate::Arguments* args) {
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

void Clipboard::WriteText(const base::string16& text, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteText(text);
}

base::string16 Clipboard::ReadRtf(mate::Arguments* args) {
  std::string data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadRTF(GetClipboardType(args), &data);
  return base::UTF8ToUTF16(data);
}

void Clipboard::WriteRtf(const std::string& text, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteRTF(text);
}

base::string16 Clipboard::ReadHtml(mate::Arguments* args) {
  base::string16 data;
  base::string16 html;
  std::string url;
  uint32_t start;
  uint32_t end;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadHTML(GetClipboardType(args), &html, &url, &start, &end);
  data = html.substr(start, end - start);
  return data;
}

void Clipboard::WriteHtml(const base::string16& html, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteHTML(html, std::string());
}

v8::Local<v8::Value> Clipboard::ReadBookmark(mate::Arguments* args) {
  base::string16 title;
  std::string url;
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(args->isolate());
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadBookmark(&title, &url);
  dict.Set("title", title);
  dict.Set("url", url);
  return dict.GetHandle();
}

void Clipboard::WriteBookmark(const base::string16& title,
                              const std::string& url,
                              mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteBookmark(title, url);
}

gfx::Image Clipboard::ReadImage(mate::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  SkBitmap bitmap = clipboard->ReadImage(GetClipboardType(args));
  return gfx::Image::CreateFrom1xBitmap(bitmap);
}

void Clipboard::WriteImage(const gfx::Image& image, mate::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardType(args));
  writer.WriteImage(image.AsBitmap());
}

#if !defined(OS_MACOSX)
void Clipboard::WriteFindText(const base::string16& text) {}
base::string16 Clipboard::ReadFindText() { return ""; }
#endif

void Clipboard::Clear(mate::Arguments* args) {
  ui::Clipboard::GetForCurrentThread()->Clear(GetClipboardType(args));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("availableFormats", &atom::api::Clipboard::AvailableFormats);
  dict.SetMethod("has", &atom::api::Clipboard::Has);
  dict.SetMethod("read", &atom::api::Clipboard::Read);
  dict.SetMethod("write", &atom::api::Clipboard::Write);
  dict.SetMethod("readText", &atom::api::Clipboard::ReadText);
  dict.SetMethod("writeText", &atom::api::Clipboard::WriteText);
  dict.SetMethod("readRTF", &atom::api::Clipboard::ReadRtf);
  dict.SetMethod("writeRTF", &atom::api::Clipboard::WriteRtf);
  dict.SetMethod("readHTML", &atom::api::Clipboard::ReadHtml);
  dict.SetMethod("writeHTML", &atom::api::Clipboard::WriteHtml);
  dict.SetMethod("readBookmark", &atom::api::Clipboard::ReadBookmark);
  dict.SetMethod("writeBookmark", &atom::api::Clipboard::WriteBookmark);
  dict.SetMethod("readImage", &atom::api::Clipboard::ReadImage);
  dict.SetMethod("writeImage", &atom::api::Clipboard::WriteImage);
  dict.SetMethod("readFindText", &atom::api::Clipboard::ReadFindText);
  dict.SetMethod("writeFindText", &atom::api::Clipboard::WriteFindText);
  dict.SetMethod("clear", &atom::api::Clipboard::Clear);

  // TODO(kevinsawicki): Remove in 2.0, deprecate before then with warnings
  dict.SetMethod("readRtf", &atom::api::Clipboard::ReadRtf);
  dict.SetMethod("writeRtf", &atom::api::Clipboard::WriteRtf);
  dict.SetMethod("readHtml", &atom::api::Clipboard::ReadHtml);
  dict.SetMethod("writeHtml", &atom::api::Clipboard::WriteHtml);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_clipboard, Initialize)
