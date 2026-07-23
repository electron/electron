// Copyright (c) 2026 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_clipboard_item.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/object_template_builder.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "shell/browser/api/electron_api_clipboard.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_constants.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/clipboard/clipboard_url_info.h"
#include "ui/base/clipboard/file_info.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_util.h"
#include "url/gurl.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace {

constexpr std::string_view kImageJpegMime = "image/jpeg";

// Coerce a ClipboardItem payload into its UTF-8 byte sequence. Per the W3C
// spec a representation's value may be a string (UTF-8 encoded) or a Blob;
// the JS facade delivers a Blob's bytes as an `ArrayBufferView`. On a type
// mismatch this throws a `TypeError` and returns `std::nullopt`.
std::optional<std::string> PayloadToUtf8(v8::Isolate* isolate,
                                         v8::Local<v8::Value> value) {
  if (value->IsArrayBufferView()) {
    auto view = value.As<v8::ArrayBufferView>();
    std::string bytes(view->ByteLength(), '\0');
    view->CopyContents(bytes.data(), bytes.size());
    return bytes;
  }
  std::string str;
  if (gin::ConvertFromV8(isolate, value, &str))
    return str;
  isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
      isolate, "ClipboardItem payload values must be a string or a Blob")));
  return std::nullopt;
}

// Write `bytes` to `writer` under the raw platform format `format` via
// `WriteUnsafeRawData`. Used by the `electron application/osclipboard;
// format="..."` MIME and the arbitrary-MIME fallback so the bytes land
// under `format` verbatim for native-app interop.
void WriteRawData(ui::ScopedClipboardWriter& writer,
                  const std::string& format,
                  base::span<const uint8_t> bytes) {
  writer.WriteUnsafeRawData(base::UTF8ToUTF16(format),
                            mojo_base::BigBuffer{bytes});
}

// Write a W3C `web `-prefixed custom format. Unlike `WriteRawData`, this
// goes through `ScopedClipboardWriter::WriteData` — Chromium's custom
// format write path — so the `org.w3.web-custom-format.map` is committed.
// That map is what lets `clipboard.read()` (via
// `ExtractCustomPlatformNames` inside `ReadAvailableStandardAndCustomFormat-
// Names`) discover the format under its `web ` MIME name.
//
// `format` is the public `web `-prefixed MIME (e.g. `web text/plain+foo`),
// but the map keys Chromium parses are the *bare* MIME type — Chromium
// re-adds the `web ` prefix on read (see `OnCustomFormatDataRead` in
// `clipboard.cc`). So strip the prefix before handing it to `WriteData`;
// the caller has already verified it.
void WriteWebCustomData(ui::ScopedClipboardWriter& writer,
                        const std::string& format,
                        base::span<const uint8_t> bytes) {
  constexpr std::string_view kPrefix{ui::kWebClipboardFormatPrefix};
  const std::string_view bare_format =
      std::string_view{format}.substr(kPrefix.size());
  const std::u16string format16 = base::UTF8ToUTF16(bare_format);
  writer.WriteData(format16, mojo_base::BigBuffer{bytes});
}

// Resolve `promise` with a Buffer constructed from `bytes`. Sets up its own
// `HandleScope` so the caller can be inside a Chromium async callback
// without one.
void ResolveAsBuffer(gin_helper::Promise<v8::Local<v8::Value>> promise,
                     base::span<const uint8_t> bytes) {
  v8::Isolate* const isolate = promise.isolate();
  v8::HandleScope scope{isolate};
  promise.Resolve(electron::Buffer::Copy(isolate, bytes).ToLocalChecked());
}

void ResolveAsBuffer(gin_helper::Promise<v8::Local<v8::Value>> promise,
                     const std::string& bytes) {
  return ResolveAsBuffer(std::move(promise), base::as_byte_span(bytes));
}

void ResolveAsBookmark(gin_helper::Promise<v8::Local<v8::Value>> promise,
                       const electron::api::BookmarkInfo& info) {
  v8::Isolate* const isolate = promise.isolate();
  v8::HandleScope scope{isolate};
  // Enter the promise's stored context before building the dictionary —
  // `isolate->GetCurrentContext()` is empty when `ui::Clipboard` reads run
  // genuinely async (e.g. ozone/wayland) with no JS frame on the stack.
  v8::Context::Scope context_scope{promise.GetContext()};
  auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("title", info.title);
  dict.Set("url", info.url);
  promise.Resolve(dict.GetHandle());
}

// Look up `format_string` in Chromium's web-custom-format mapping, then
// read the underlying bytes.
void ReadRawAndResolve(gin_helper::Promise<v8::Local<v8::Value>> promise,
                       ui::ClipboardBuffer buffer,
                       std::string format_string) {
  ui::Clipboard::GetForCurrentThread()->ExtractCustomPlatformNames(
      buffer, /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](gin_helper::Promise<v8::Local<v8::Value>> promise,
             std::string format_string,
             std::map<std::string, std::string> custom_format_names) {
            const auto it = custom_format_names.find(format_string);
            const std::string& platform_format =
                it != custom_format_names.end() ? it->second : format_string;
            ui::Clipboard::GetForCurrentThread()->ReadData(
                ui::ClipboardFormatType::CustomPlatformType(platform_format),
                /* data_dst = */ std::nullopt,
                base::BindOnce(
                    [](gin_helper::Promise<v8::Local<v8::Value>> promise,
                       std::string data) {
                      ResolveAsBuffer(std::move(promise), data);
                    },
                    std::move(promise)));
          },
          std::move(promise), std::move(format_string)));
}

// Per-MIME read dispatch — the single source of truth for how a clipboard
// payload is decoded into a JS-visible value. Used exclusively by
// `ClipboardItem::GetType` on a read-side item to fetch the bytes for a
// given MIME on demand.
v8::Local<v8::Promise> ReadMime(ui::ClipboardBuffer buffer,
                                const std::string& mime,
                                v8::Isolate* isolate) {
  namespace cu = electron::api::clipboard_util;

  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // W3C "web "-prefixed custom formats — preserve the bytes verbatim
  // through the raw read path so payloads survive byte-for-byte.
  if (mime.starts_with(ui::kWebClipboardFormatPrefix)) {
    ReadRawAndResolve(std::move(promise), buffer, mime);
    return handle;
  }

  // `electron application/osclipboard;format="<name>"` is a raw read of the
  // platform-specific clipboard format `<name>`.
  if (auto os_format = cu::ParseOSClipboardFormat(mime)) {
    ReadRawAndResolve(std::move(promise), buffer, std::move(*os_format));
    return handle;
  }

  if (mime == ui::kMimeTypePlainText) {
    ui::Clipboard::GetForCurrentThread()->ReadText(
        buffer, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               ui::ClipboardBuffer buf, std::u16string text) {
#if BUILDFLAG(IS_WIN)
              if (text.empty()) {
                ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
                    buf, /* data_dst = */ std::nullopt,
                    base::BindOnce(
                        [](gin_helper::Promise<v8::Local<v8::Value>> p,
                           std::string ascii) {
                          ResolveAsBuffer(std::move(p), ascii);
                        },
                        std::move(promise)));
                return;
              }
#endif
              ResolveAsBuffer(std::move(promise), base::UTF16ToUTF8(text));
            },
            std::move(promise), buffer));
    return handle;
  }

  if (mime == ui::kMimeTypeHtml) {
    ui::Clipboard::GetForCurrentThread()->ReadHTML(
        buffer, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               std::u16string markup, GURL src_url, uint32_t fragment_start,
               uint32_t fragment_end) {
              std::u16string fragment =
                  markup.substr(fragment_start, fragment_end - fragment_start);
              ResolveAsBuffer(std::move(promise), base::UTF16ToUTF8(fragment));
            },
            std::move(promise)));
    return handle;
  }

  if (mime == ui::kMimeTypeRtf) {
    ui::Clipboard::GetForCurrentThread()->ReadRTF(
        buffer, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               std::string rtf) { ResolveAsBuffer(std::move(promise), rtf); },
            std::move(promise)));
    return handle;
  }

  if (mime.starts_with(cu::kImagePrefix)) {
    // `ReadPng` schedules PNG decoding on the thread pool, which requires
    // the browser to be ready.
    if (!electron::Browser::Get()->is_ready()) {
      promise.RejectWithErrorMessage(
          "clipboard.read of image/* is available only after app ready");
      return handle;
    }
    const bool want_jpeg = (mime == kImageJpegMime);
    ui::Clipboard::GetForCurrentThread()->ReadPng(
        buffer, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               bool want_jpeg, const std::vector<uint8_t>& png) {
              SkBitmap bitmap = gfx::PNGCodec::Decode(png);
              if (bitmap.isNull()) {
                ResolveAsBuffer(std::move(promise),
                                base::span<const uint8_t>{});
                return;
              }
              std::optional<std::vector<uint8_t>> encoded;
              if (want_jpeg) {
                encoded = gfx::JPEG1xEncodedDataFromImage(
                    gfx::Image::CreateFrom1xBitmap(bitmap), 100);
              } else {
                encoded = png;
              }
              ResolveAsBuffer(std::move(promise),
                              encoded.has_value()
                                  ? base::span<const uint8_t>(*encoded)
                                  : base::span<const uint8_t>{});
            },
            std::move(promise), want_jpeg));
    return handle;
  }

  if (mime == cu::kBookmarkMime) {
    // Unlike every other MIME type, `getType('electron application/bookmark')`
    // resolves to a plain `{ title, url }` object rather than a Buffer — a
    // bookmark is inherently structured data, so this is far easier to use
    // than a serialized byte payload.
    ui::Clipboard::GetForCurrentThread()->ReadURL(
        /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               ui::ClipboardUrlInfo url_info) {
              ResolveAsBookmark(std::move(promise),
                                electron::api::BookmarkInfo{
                                    url_info.title, url_info.url.spec()});
            },
            std::move(promise)));
    return handle;
  }

#if BUILDFLAG(IS_MAC)
  if (mime == cu::kFindTextMime) {
    // `NSFindPasteboard` is synchronous; resolve immediately.
    ResolveAsBuffer(
        std::move(promise),
        base::UTF16ToUTF8(electron::api::Clipboard::ReadFindText()));
    return handle;
  }
#endif

  if (mime == ui::kMimeTypeUriList) {
    // `text/uri-list` maps to the OS "copied files" format. Read the file
    // list via `ReadFilenames` — which yields absolute paths on the
    // privileged main-process clipboard — and serialize it back to an
    // RFC 2483 `file://` URI list.
    ui::Clipboard::GetForCurrentThread()->ReadFilenames(
        buffer, /* data_dst = */ std::nullopt,
        base::BindOnce(
            [](gin_helper::Promise<v8::Local<v8::Value>> promise,
               std::vector<ui::FileInfo> files) {
              ResolveAsBuffer(std::move(promise),
                              ui::FileInfosToURIList(files));
            },
            std::move(promise)));
    return handle;
  }

  // Any other MIME falls through to a raw read so arbitrary user-defined
  // formats round-trip.
  ReadRawAndResolve(std::move(promise), buffer, mime);
  return handle;
}

}  // namespace

namespace electron::api {

const gin::WrapperInfo ClipboardItem::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronClipboardItem);

ClipboardItem::ClipboardItem(v8::Isolate* /*isolate*/,
                             ui::ClipboardBuffer buffer,
                             std::vector<std::string> types)
    : types_{std::move(types)}, is_read_side_{true}, buffer_{buffer} {}

ClipboardItem::ClipboardItem(gin::Arguments* args)
    : is_read_side_{false}, buffer_{ui::ClipboardBuffer::kCopyPaste} {
  v8::Isolate* const isolate = args->isolate();

  // W3C `ClipboardItem(items, options?)` requires `items` as a plain
  // object whose keys are MIME types and whose values are the payloads.
  v8::Local<v8::Value> items_value;
  if (!args->GetNext(&items_value) || !items_value->IsObject() ||
      items_value->IsArray()) {
    args->ThrowTypeError(
        "Failed to construct 'ClipboardItem': 1 argument required — an "
        "object mapping MIME types to payloads.");
    return;
  }
  // The optional `options` argument is accepted but ignored — the W3C
  // `ClipboardItemOptions` (presentationStyle) isn't surfaced here yet.
  v8::Local<v8::Value> options_value;
  args->GetNext(&options_value);

  v8::Local<v8::Object> data = items_value.As<v8::Object>();

  // Walk the user's `{ [mime]: payload }` object and parse each entry into
  // the typed `ClipboardItemPayload` variant. Per the W3C spec a
  // representation's value may be a string for *any* MIME type (it is UTF-8
  // encoded into the payload bytes) or a Blob; the bookmark format instead
  // takes a `{ title, url }` object. A mismatch (e.g. a bookmark MIME
  // without an object, or a value that is neither a string nor a Blob)
  // raises a `TypeError` via `isolate->ThrowException` — `ClipboardItem::New`
  // checks for a pending exception after `MakeGarbageCollected` returns and
  // propagates.
  //
  // The JS facade resolves each Blob to a Buffer before handing it here, so
  // native sees a string or an `ArrayBufferView`. User-facing error strings
  // still say "Blob" because that is what the caller passed.
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Array> keys;
  if (!data->GetOwnPropertyNames(context).ToLocal(&keys))
    return;

  for (uint32_t i = 0; i < keys->Length(); ++i) {
    v8::Local<v8::Value> key, value;
    if (!keys->Get(context, i).ToLocal(&key)) {
      isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
          isolate,
          "Failed to construct 'ClipboardItem': unable to read an items "
          "object key.")));
      return;
    }
    if (!data->Get(context, key).ToLocal(&value)) {
      isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
          isolate,
          "Failed to construct 'ClipboardItem': unable to read the payload "
          "for a MIME entry.")));
      return;
    }
    std::string mime;
    if (!gin::ConvertFromV8(isolate, key, &mime)) {
      isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
          isolate,
          "Failed to construct 'ClipboardItem': MIME type keys must be "
          "strings.")));
      return;
    }

    if (mime == electron::api::clipboard_util::kBookmarkMime) {
      // Bookmark MIME takes a structured `{ title, url }` object.
      if (!value->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(gin::StringToV8(
            isolate,
            "ClipboardItem payload for `electron application/bookmark` "
            "must be an object with `title` and `url` properties")));
        return;
      }
      gin_helper::Dictionary dict{isolate, value.As<v8::Object>()};
      BookmarkInfo info;
      dict.Get("title", &info.title);
      dict.Get("url", &info.url);
      payloads_.emplace(mime, std::move(info));
      types_.push_back(std::move(mime));
      continue;
    }

    if (mime == ui::kMimeTypePlainText || mime == ui::kMimeTypeHtml ||
        mime == ui::kMimeTypeRtf ||
        mime == electron::api::clipboard_util::kFindTextMime) {
      // Text-typed MIMEs are stored as UTF-16 text.
      auto utf8 = PayloadToUtf8(isolate, value);
      if (!utf8)
        return;
      payloads_.emplace(mime, base::UTF8ToUTF16(*utf8));
      types_.push_back(std::move(mime));
      continue;
    }

    // Everything else is stored as the raw payload bytes.
    auto utf8 = PayloadToUtf8(isolate, value);
    if (!utf8)
      return;
    payloads_.emplace(mime, std::vector<uint8_t>(utf8->begin(), utf8->end()));
    types_.push_back(std::move(mime));
  }
}

ClipboardItem::~ClipboardItem() = default;

const gin::WrapperInfo* ClipboardItem::wrapper_info() const {
  return &kWrapperInfo;
}

const char* ClipboardItem::GetHumanReadableName() const {
  return "Electron / ClipboardItem";
}

// static
ClipboardItem* ClipboardItem::CreateForRead(v8::Isolate* isolate,
                                            ui::ClipboardBuffer buffer,
                                            std::vector<std::string> types) {
  return cppgc::MakeGarbageCollected<ClipboardItem>(
      isolate->GetCppHeap()->GetAllocationHandle(), isolate, buffer,
      std::move(types));
}

// static
ClipboardItem* ClipboardItem::New(gin::Arguments* args) {
  // `gin_helper::Constructible<T>::GetConstructor` binds `&T::New` as the
  // JS-callable constructor entry point, so this stub is required. All
  // argument validation and parsing happens inside the constructor.
  // Returning `nullptr` when an exception was thrown lets gin propagate
  // the `TypeError`; without the check, gin would see a valid pointer
  // and return the (partially-constructed) item instead.
  v8::Isolate* const isolate = args->isolate();
  ClipboardItem* item = cppgc::MakeGarbageCollected<ClipboardItem>(
      isolate->GetCppHeap()->GetAllocationHandle(), args);
  return isolate->HasPendingException() ? nullptr : item;
}

// static
void ClipboardItem::FillObjectTemplate(v8::Isolate* isolate,
                                       v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetProperty("types", &ClipboardItem::GetTypes)
      .SetMethod("getType", &ClipboardItem::GetType)
      .Build();
}

void ClipboardItem::WriteTo(ui::ScopedClipboardWriter& writer) const {
  // Read-side ClipboardItems hold no payloads — `Clipboard::Write` rejects
  // them before getting here, but be defensive in case a future caller
  // passes one through.
  if (is_read_side_)
    return;

  for (const auto& [mime, payload] : payloads_) {
    if (std::holds_alternative<BookmarkInfo>(payload)) {
      const auto& bm = std::get<BookmarkInfo>(payload);
      writer.WriteURL(
          ui::ClipboardUrlInfo{.url = GURL(bm.url), .title = bm.title});
      continue;
    }

    if (std::holds_alternative<std::u16string>(payload)) {
      const auto& text = std::get<std::u16string>(payload);
      if (mime == ui::kMimeTypePlainText) {
        writer.WriteText(text);
      } else if (mime == ui::kMimeTypeHtml) {
        writer.WriteHTML(text, std::string());
      } else if (mime == ui::kMimeTypeRtf) {
        writer.WriteRTF(base::UTF16ToUTF8(text));
      } else if (mime == clipboard_util::kFindTextMime) {
        // The macOS find pasteboard is a *separate* NSPasteboard from
        // the system clipboard. Commit directly here — it doesn't need
        // to share the `ScopedClipboardWriter`'s atomic-commit
        // lifetime. No-op on non-Mac platforms.
        Clipboard::WriteFindText(text);
      } else {
        NOTREACHED();
      }

      continue;
    }

    // std::vector<uint8_t> — raw byte payload.
    const auto& bytes = std::get<std::vector<uint8_t>>(payload);
    const base::span<const uint8_t> span{bytes};
    if (mime.starts_with(ui::kWebClipboardFormatPrefix)) {
      WriteWebCustomData(writer, mime, span);
    } else if (auto os_format = clipboard_util::ParseOSClipboardFormat(mime)) {
      WriteRawData(writer, *os_format, span);
    } else if (mime == ui::kMimeTypeUriList) {
      // `text/uri-list` maps to the OS "copied files" format. The payload is
      // an RFC 2483 `file://` URI list, which
      // `ScopedClipboardWriter::WriteFilenames` consumes directly.
      writer.WriteFilenames(std::string(bytes.begin(), bytes.end()));
    } else if (mime.starts_with(clipboard_util::kImagePrefix)) {
      // Decode the supplied PNG/JPEG bytes via the same auto-detecting
      // helper `nativeImage.createFromBuffer` uses.
      gfx::ImageSkia image_skia;
      if (electron::util::AddImageSkiaRepFromBuffer(&image_skia, span, 0, 0,
                                                    1.0)) {
        SkBitmap orig = gfx::Image(image_skia).AsBitmap();
        SkBitmap bmp;
        if (bmp.tryAllocPixels(orig.info()) &&
            orig.readPixels(bmp.info(), bmp.getPixels(), bmp.rowBytes(), 0,
                            0)) {
          writer.WriteImage(bmp);
        }
      }
    } else {
      // Fallback for arbitrary MIME types — write the bytes verbatim
      // under their MIME name.
      WriteRawData(writer, mime, span);
    }
  }
}

v8::Local<v8::Promise> ClipboardItem::GetType(const std::string& mime,
                                              v8::Isolate* const isolate) {
  // Read-side: delegate to the per-MIME async read path bound to this
  // item's source buffer. No v8 closures are captured at `clipboard.read()`
  // time — the platform clipboard is queried lazily here.
  if (is_read_side_)
    return ReadMime(buffer_, mime, isolate);

  // Constructed: resolve with the pre-parsed payload for this MIME,
  // mirroring the W3C `ClipboardItem.getType()` shape. Reject with the
  // W3C-style error string if the type isn't present.
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto it = payloads_.find(mime);
  if (it == payloads_.end()) {
    promise.RejectWithErrorMessage("The type '" + mime +
                                   "' was not found in the ClipboardItem");
    return handle;
  }

  std::visit(
      [&](const auto& payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, BookmarkInfo>) {
          ResolveAsBookmark(std::move(promise), payload);
        } else if constexpr (std::is_same_v<T, std::u16string>) {
          promise.Resolve(gin::ConvertToV8(isolate, payload));
        } else {  // std::vector<uint8_t>
          ResolveAsBuffer(std::move(promise),
                          base::span<const uint8_t>(payload));
        }
      },
      it->second);

  return handle;
}

}  // namespace electron::api

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> module,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Function> constructor =
      electron::api::ClipboardItem::GetConstructor(
          isolate, context, &electron::api::ClipboardItem::kWrapperInfo);
  // Replace the binding's effective exports with the `ClipboardItem`
  // constructor itself, so `process._linkedBinding(
  // 'electron_browser_clipboard_item')` returns the W3C-style class
  // directly. node re-reads `module.exports` after `Initialize` returns
  // (see `GetLinkedBinding` in `node_binding.cc`) and uses whatever value
  // it finds there as the binding result.
  module.As<v8::Object>()
      ->Set(context, gin::StringToV8(isolate, "exports"), constructor)
      .Check();
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_clipboard_item, Initialize)
