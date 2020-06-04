// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include "base/strings/utf_string_conversions.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"

namespace electron {

namespace api {

ui::ClipboardBuffer Clipboard::GetClipboardBuffer(gin_helper::Arguments* args) {
  std::string type;
  if (args->GetNext(&type) && type == "selection")
    return ui::ClipboardBuffer::kSelection;
  else
    return ui::ClipboardBuffer::kCopyPaste;
}

std::vector<base::string16> Clipboard::AvailableFormats(
    gin_helper::Arguments* args) {
  std::vector<base::string16> format_types;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadAvailableTypes(GetClipboardBuffer(args), &format_types);
  return format_types;
}

bool Clipboard::Has(const std::string& format_string,
                    gin_helper::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardFormatType format(
      ui::ClipboardFormatType::GetType(format_string));
  return clipboard->IsFormatAvailable(format, GetClipboardBuffer(args));
}

std::string Clipboard::Read(const std::string& format_string) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardFormatType format(
      ui::ClipboardFormatType::GetType(format_string));

  std::string data;
  clipboard->ReadData(format, &data);
  return data;
}

v8::Local<v8::Value> Clipboard::ReadBuffer(const std::string& format_string,
                                           gin_helper::Arguments* args) {
  std::string data = Read(format_string);
  return node::Buffer::Copy(args->isolate(), data.data(), data.length())
      .ToLocalChecked();
}

void Clipboard::WriteBuffer(const std::string& format,
                            const v8::Local<v8::Value> buffer,
                            gin_helper::Arguments* args) {
  if (!node::Buffer::HasInstance(buffer)) {
    args->ThrowError("buffer must be a node Buffer");
    return;
  }

  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  base::span<const uint8_t> payload_span(
      reinterpret_cast<const uint8_t*>(node::Buffer::Data(buffer)),
      node::Buffer::Length(buffer));
  writer.WriteData(
      base::UTF8ToUTF16(ui::ClipboardFormatType::GetType(format).Serialize()),
      mojo_base::BigBuffer(payload_span));
}

void Clipboard::Write(const gin_helper::Dictionary& data,
                      gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
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

base::string16 Clipboard::ReadText(gin_helper::Arguments* args) {
  base::string16 data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  auto type = GetClipboardBuffer(args);
  if (clipboard->IsFormatAvailable(ui::ClipboardFormatType::GetPlainTextType(),
                                   type)) {
    clipboard->ReadText(type, &data);
  } else {
#if defined(OS_WIN)
    if (clipboard->IsFormatAvailable(
            ui::ClipboardFormatType::GetPlainTextAType(), type)) {
      std::string result;
      clipboard->ReadAsciiText(type, &result);
      data = base::ASCIIToUTF16(result);
    }
#endif
  }
  return data;
}

void Clipboard::WriteText(const base::string16& text,
                          gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteText(text);
}

base::string16 Clipboard::ReadRTF(gin_helper::Arguments* args) {
  std::string data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadRTF(GetClipboardBuffer(args), &data);
  return base::UTF8ToUTF16(data);
}

void Clipboard::WriteRTF(const std::string& text, gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteRTF(text);
}

base::string16 Clipboard::ReadHTML(gin_helper::Arguments* args) {
  base::string16 data;
  base::string16 html;
  std::string url;
  uint32_t start;
  uint32_t end;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadHTML(GetClipboardBuffer(args), &html, &url, &start, &end);
  data = html.substr(start, end - start);
  return data;
}

void Clipboard::WriteHTML(const base::string16& html,
                          gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteHTML(html, std::string());
}

v8::Local<v8::Value> Clipboard::ReadBookmark(gin_helper::Arguments* args) {
  base::string16 title;
  std::string url;
  gin_helper::Dictionary dict =
      gin_helper::Dictionary::CreateEmpty(args->isolate());
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  clipboard->ReadBookmark(&title, &url);
  dict.Set("title", title);
  dict.Set("url", url);
  return dict.GetHandle();
}

void Clipboard::WriteBookmark(const base::string16& title,
                              const std::string& url,
                              gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteBookmark(title, url);
}

gfx::Image Clipboard::ReadImage(gin_helper::Arguments* args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  base::Optional<gfx::Image> image;
  clipboard->ReadImage(
      GetClipboardBuffer(args),
      base::Bind(
          [](base::Optional<gfx::Image>* image, const SkBitmap& result) {
            image->emplace(gfx::Image::CreateFrom1xBitmap(result));
          },
          &image));
  DCHECK(image.has_value());
  return image.value();
}

void Clipboard::WriteImage(const gfx::Image& image,
                           gin_helper::Arguments* args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  SkBitmap orig = image.AsBitmap();
  SkBitmap bmp;

  if (bmp.tryAllocPixels(orig.info()) &&
      orig.readPixels(bmp.info(), bmp.getPixels(), bmp.rowBytes(), 0, 0)) {
    writer.WriteImage(bmp);
  }
}

#if !defined(OS_MACOSX)
void Clipboard::WriteFindText(const base::string16& text) {}
base::string16 Clipboard::ReadFindText() {
  return base::string16();
}
#endif

void Clipboard::Clear(gin_helper::Arguments* args) {
  ui::Clipboard::GetForCurrentThread()->Clear(GetClipboardBuffer(args));
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("availableFormats",
                 &electron::api::Clipboard::AvailableFormats);
  dict.SetMethod("has", &electron::api::Clipboard::Has);
  dict.SetMethod("read", &electron::api::Clipboard::Read);
  dict.SetMethod("write", &electron::api::Clipboard::Write);
  dict.SetMethod("readText", &electron::api::Clipboard::ReadText);
  dict.SetMethod("writeText", &electron::api::Clipboard::WriteText);
  dict.SetMethod("readRTF", &electron::api::Clipboard::ReadRTF);
  dict.SetMethod("writeRTF", &electron::api::Clipboard::WriteRTF);
  dict.SetMethod("readHTML", &electron::api::Clipboard::ReadHTML);
  dict.SetMethod("writeHTML", &electron::api::Clipboard::WriteHTML);
  dict.SetMethod("readBookmark", &electron::api::Clipboard::ReadBookmark);
  dict.SetMethod("writeBookmark", &electron::api::Clipboard::WriteBookmark);
  dict.SetMethod("readImage", &electron::api::Clipboard::ReadImage);
  dict.SetMethod("writeImage", &electron::api::Clipboard::WriteImage);
  dict.SetMethod("readFindText", &electron::api::Clipboard::ReadFindText);
  dict.SetMethod("writeFindText", &electron::api::Clipboard::WriteFindText);
  dict.SetMethod("readBuffer", &electron::api::Clipboard::ReadBuffer);
  dict.SetMethod("writeBuffer", &electron::api::Clipboard::WriteBuffer);
  dict.SetMethod("clear", &electron::api::Clipboard::Clear);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_clipboard, Initialize)
