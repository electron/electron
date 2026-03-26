// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <map>

#include "base/containers/flat_set.h"
#include "base/containers/to_vector.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/process_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/clipboard/clipboard_url_info.h"
#include "ui/base/clipboard/file_info.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace {

[[nodiscard]] ui::ClipboardBuffer GetClipboardBuffer(gin::Arguments* args) {
  std::string type;
  return args->GetNext(&type) && type == "selection"
             ? ui::ClipboardBuffer::kSelection
             : ui::ClipboardBuffer::kCopyPaste;
}

bool IsFormatAvailable(ui::Clipboard* clipboard,
                       const ui::ClipboardFormatType& format,
                       ui::ClipboardBuffer buffer) {
  base::flat_set<ui::ClipboardFormatType> formats;
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->GetAllAvailableFormats(
      buffer, /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](base::flat_set<ui::ClipboardFormatType>* out,
             base::OnceClosure quit,
             base::flat_set<ui::ClipboardFormatType> result) {
            *out = std::move(result);
            std::move(quit).Run();
          },
          &formats, run_loop.QuitClosure()));
  run_loop.Run();
  return formats.contains(format);
}

}  // namespace

namespace electron::api {

std::vector<std::u16string> Clipboard::AvailableFormats(
    gin::Arguments* const args) {
  std::vector<std::u16string> format_types;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->ReadAvailableTypes(
      GetClipboardBuffer(args),
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::vector<std::u16string>* out, base::OnceClosure quit,
             std::vector<std::u16string> result) {
            *out = std::move(result);
            std::move(quit).Run();
          },
          &format_types, run_loop.QuitClosure()));
  run_loop.Run();

  return format_types;
}

bool Clipboard::Has(const std::string& format_string,
                    gin::Arguments* const args) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  ui::ClipboardFormatType format =
      ui::ClipboardFormatType::CustomPlatformType(format_string);
  if (format.GetName().empty())
    format = ui::ClipboardFormatType::CustomPlatformType(format_string);
  return IsFormatAvailable(clipboard, format, GetClipboardBuffer(args));
}

std::string Clipboard::Read(const std::string& format_string) {
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  // Prefer raw platform format names
  ui::ClipboardFormatType rawFormat(
      ui::ClipboardFormatType::CustomPlatformType(format_string));
  bool rawFormatAvailable =
      IsFormatAvailable(clipboard, rawFormat, ui::ClipboardBuffer::kCopyPaste);
#if BUILDFLAG(IS_LINUX)
  if (!rawFormatAvailable) {
    rawFormatAvailable = IsFormatAvailable(clipboard, rawFormat,
                                           ui::ClipboardBuffer::kSelection);
  }
#endif
  if (rawFormatAvailable) {
    std::string data;

    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    clipboard->ReadData(
        rawFormat,
        /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](std::string* out, base::OnceClosure quit, std::string result) {
              *out = std::move(result);
              std::move(quit).Run();
            },
            &data, run_loop.QuitClosure()));
    run_loop.Run();

    return data;
  }
  // Otherwise, resolve custom format names
  std::map<std::string, std::string> custom_format_names;
  {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    clipboard->ExtractCustomPlatformNames(
        ui::ClipboardBuffer::kCopyPaste, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](std::map<std::string, std::string>* out, base::OnceClosure quit,
               std::map<std::string, std::string> result) {
              *out = std::move(result);
              std::move(quit).Run();
            },
            &custom_format_names, run_loop.QuitClosure()));
    run_loop.Run();
  }
#if BUILDFLAG(IS_LINUX)
  if (!custom_format_names.contains(format_string)) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    clipboard->ExtractCustomPlatformNames(
        ui::ClipboardBuffer::kSelection, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](std::map<std::string, std::string>* out, base::OnceClosure quit,
               std::map<std::string, std::string> result) {
              *out = std::move(result);
              std::move(quit).Run();
            },
            &custom_format_names, run_loop.QuitClosure()));
    run_loop.Run();
  }
#endif

  ui::ClipboardFormatType format;
  if (custom_format_names.contains(format_string)) {
    format =
        ui::ClipboardFormatType(ui::ClipboardFormatType::CustomPlatformType(
            custom_format_names[format_string]));
  } else {
    format = ui::ClipboardFormatType(
        ui::ClipboardFormatType::CustomPlatformType(format_string));
  }
  std::string data;

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->ReadData(
      format,
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::string* out, base::OnceClosure quit, std::string result) {
            *out = std::move(result);
            std::move(quit).Run();
          },
          &data, run_loop.QuitClosure()));
  run_loop.Run();

  return data;
}

v8::Local<v8::Value> Clipboard::ReadBuffer(v8::Isolate* const isolate,
                                           const std::string& format_string) {
  std::string data = Read(format_string);
  return electron::Buffer::Copy(isolate, data).ToLocalChecked();
}

void Clipboard::WriteBuffer(const std::string& format,
                            const v8::Local<v8::Value> buffer,
                            gin::Arguments* const args) {
  if (!node::Buffer::HasInstance(buffer)) {
    args->ThrowTypeError("buffer must be a node Buffer");
    return;
  }

  CHECK(buffer->IsArrayBufferView());
  v8::Local<v8::ArrayBufferView> buffer_view = buffer.As<v8::ArrayBufferView>();
  const size_t n_bytes = buffer_view->ByteLength();
  mojo_base::BigBuffer big_buffer{n_bytes};
  [[maybe_unused]] const size_t n_got =
      buffer_view->CopyContents(big_buffer.data(), n_bytes);
  DCHECK_EQ(n_got, n_bytes);

  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteUnsafeRawData(base::UTF8ToUTF16(format), std::move(big_buffer));
}

void Clipboard::Write(const gin_helper::Dictionary& data,
                      gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  std::u16string text, html, bookmark;
  gfx::Image image;

  if (data.Get("text", &text)) {
    writer.WriteText(text);

    if (data.Get("bookmark", &bookmark))
      writer.WriteURL(
          ui::ClipboardUrlInfo{.url = GURL(text), .title = bookmark});
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

std::u16string Clipboard::ReadText(gin::Arguments* const args) {
  std::u16string data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  auto type = GetClipboardBuffer(args);
  if (IsFormatAvailable(clipboard, ui::ClipboardFormatType::PlainTextType(),
                        type)) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    clipboard->ReadText(type,
                        /* data_dst = */ std::nullopt,
                        base::BindOnce(
                            [](std::u16string* out, base::OnceClosure quit,
                               std::u16string result) {
                              *out = std::move(result);
                              std::move(quit).Run();
                            },
                            &data, run_loop.QuitClosure()));
    run_loop.Run();
  } else {
#if BUILDFLAG(IS_WIN)
    if (IsFormatAvailable(clipboard, ui::ClipboardFormatType::PlainTextAType(),
                          type)) {
      std::string result;
      base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
      clipboard->ReadAsciiText(
          type,
          /* data_dst = */ std::nullopt,
          base::BindOnce(
              [](std::string* out, base::OnceClosure quit, std::string value) {
                *out = std::move(value);
                std::move(quit).Run();
              },
              &result, run_loop.QuitClosure()));
      run_loop.Run();
      data = base::ASCIIToUTF16(result);
    }
#endif
  }
  return data;
}

void Clipboard::WriteText(const std::u16string& text,
                          gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteText(text);
}

std::u16string Clipboard::ReadRTF(gin::Arguments* const args) {
  std::string data;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->ReadRTF(
      GetClipboardBuffer(args),
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::string* out, base::OnceClosure quit, std::string result) {
            *out = std::move(result);
            std::move(quit).Run();
          },
          &data, run_loop.QuitClosure()));
  run_loop.Run();

  return base::UTF8ToUTF16(data);
}

void Clipboard::WriteRTF(const std::string& text, gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteRTF(text);
}

std::u16string Clipboard::ReadHTML(gin::Arguments* const args) {
  std::u16string html;
  uint32_t start = 0;
  uint32_t end = 0;
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->ReadHTML(
      GetClipboardBuffer(args),
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::u16string* out_html, uint32_t* out_start, uint32_t* out_end,
             base::OnceClosure quit, std::u16string markup, GURL src_url,
             uint32_t fragment_start, uint32_t fragment_end) {
            *out_html = std::move(markup);
            *out_start = fragment_start;
            *out_end = fragment_end;
            std::move(quit).Run();
          },
          &html, &start, &end, run_loop.QuitClosure()));
  run_loop.Run();

  return html.substr(start, end - start);
}

void Clipboard::WriteHTML(const std::u16string& html,
                          gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteHTML(html, std::string());
}

v8::Local<v8::Value> Clipboard::ReadBookmark(v8::Isolate* const isolate) {
  std::u16string title;
  GURL url;
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  clipboard->ReadURL(
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::u16string* out_title, GURL* out_url, base::OnceClosure quit,
             ui::ClipboardUrlInfo url_info) {
            *out_title = std::move(url_info.title);
            *out_url = std::move(url_info.url);
            std::move(quit).Run();
          },
          &title, &url, run_loop.QuitClosure()));
  run_loop.Run();

  dict.Set("title", title);
  dict.Set("url", url.spec());
  return dict.GetHandle();
}

void Clipboard::WriteBookmark(const std::u16string& title,
                              const std::string& url,
                              gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  writer.WriteURL(ui::ClipboardUrlInfo{.url = GURL(url), .title = title});
}

gfx::Image Clipboard::ReadImage(gin::Arguments* const args) {
  // The ReadPng uses thread pool which requires app ready.
  if (IsBrowserProcess() && !Browser::Get()->is_ready()) {
    gin_helper::ErrorThrower{args->isolate()}.ThrowError(
        "clipboard.readImage is available only after app ready in the main "
        "process");
    return {};
  }

  ui::Clipboard* clipboard = ui::Clipboard::GetForCurrentThread();
  std::optional<gfx::Image> image;

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::RepeatingClosure callback = run_loop.QuitClosure();
  clipboard->ReadPng(
      GetClipboardBuffer(args),
      /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](std::optional<gfx::Image>* image, base::RepeatingClosure cb,
             const std::vector<uint8_t>& result) {
            SkBitmap bitmap = gfx::PNGCodec::Decode(result);
            if (bitmap.isNull()) {
              image->emplace();
            } else {
              image->emplace(gfx::Image::CreateFrom1xBitmap(bitmap));
            }
            std::move(cb).Run();
          },
          &image, std::move(callback)));
  run_loop.Run();

  DCHECK(image.has_value());
  return image.value();
}

void Clipboard::WriteImage(const gfx::Image& image,
                           gin::Arguments* const args) {
  ui::ScopedClipboardWriter writer(GetClipboardBuffer(args));
  SkBitmap orig = image.AsBitmap();
  SkBitmap bmp;

  if (bmp.tryAllocPixels(orig.info()) &&
      orig.readPixels(bmp.info(), bmp.getPixels(), bmp.rowBytes(), 0, 0)) {
    writer.WriteImage(bmp);
  }
}

#if !BUILDFLAG(IS_MAC)
void Clipboard::WriteFindText(const std::u16string& text) {}
std::u16string Clipboard::ReadFindText() {
  return {};
}
#endif

void Clipboard::Clear(gin::Arguments* const args) {
  ui::Clipboard::GetForCurrentThread()->Clear(GetClipboardBuffer(args));
}

// This exists for testing purposes ONLY.
void Clipboard::WriteFilesForTesting(const std::vector<base::FilePath>& files) {
  auto to_info = [](const auto& p) { return ui::FileInfo{p, p.BaseName()}; };
  auto file_infos = base::ToVector(files, to_info);
  ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
  writer.WriteFilenames(ui::FileInfosToURIList(file_infos));
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  gin_helper::Dictionary dict{isolate, exports};
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
  dict.SetMethod("_writeFilesForTesting",
                 &electron::api::Clipboard::WriteFilesForTesting);
  dict.SetMethod("clear", &electron::api::Clipboard::Clear);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_clipboard, Initialize)
