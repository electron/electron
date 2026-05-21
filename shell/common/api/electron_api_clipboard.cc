// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_clipboard.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/function_template.h"
#include "shell/common/api/electron_api_clipboard_item.h"
#include "shell/common/gin_converters/clipboard_item_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard_constants.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/clipboard/clipboard_url_info.h"
#include "ui/base/clipboard/file_info.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace {

// `ClipboardFormatType`s that `ReadAvailableStandardAndCustomFormatNames`
// already surfaces under a MIME name. The union below covers every
// standard format chromium's per-platform `GetStandardFormats` impl
// checks for:
//   * macOS (`clipboard_mac.mm`)  — PlainText, Html, Svg, Rtf, Filenames,
//                                    Png (via NSImage detection)
//   * Windows (`clipboard_win.cc`) — PlainTextA, Html, Svg, Rtf,
//                                    CF_DIB→Png, CFHDrop→Filenames
//   * Linux/Ozone (`clipboard_ozone.cc`, via `GetStandardFormatsFromMime-
//                                    Types`) — PlainText, Html, Svg, Rtf,
//                                    Bitmap, Filenames
// When merging the raw platform-format names reported by
// `GetAllAvailableFormats` into the `clipboard.read()` `types` list,
// these are skipped so we don't surface, e.g., `text/plain` *and*
// `electron application/osclipboard;format="public.utf8-plain-text"` for
// the same content.
bool IsStandardClipboardFormat(const ui::ClipboardFormatType& fmt) {
  static const base::NoDestructor<base::flat_set<ui::ClipboardFormatType>>
      kStandardFormats({
          ui::ClipboardFormatType::PlainTextType(),
          ui::ClipboardFormatType::HtmlType(),
          ui::ClipboardFormatType::SvgType(),
          ui::ClipboardFormatType::RtfType(),
          ui::ClipboardFormatType::PngType(),
          ui::ClipboardFormatType::BitmapType(),
          ui::ClipboardFormatType::FilenamesType(),
#if BUILDFLAG(IS_WIN)
          ui::ClipboardFormatType::PlainTextAType(),
          ui::ClipboardFormatType::CFHDropType(),
#endif
      });
  return kStandardFormats->contains(fmt);
}

// Build `electron application/osclipboard;format="<name>"` from a raw
// platform clipboard format name — the inverse of
// `clipboard_util::ParseOSClipboardFormat`.
std::string BuildOSClipboardFormat(const std::string& name) {
  return base::StringPrintf(
      "%s;format=\"%s\"", electron::api::clipboard_util::kOSClipboardMimePrefix,
      name);
}

using TypesCallback = base::OnceCallback<void(std::vector<std::string>)>;

// Async pipeline that aggregates the full list of MIME types currently
// available on `buffer`, then invokes `on_done` with the merged list.
// Used by both `Clipboard::Read` (to construct a `ClipboardItem`) and
// `Clipboard::Has` (to test membership) so the two stay consistent. The
// list is assembled from three chromium calls, in chronological order:
//
//   * `ReadAvailableStandardAndCustomFormatNames` — standard MIME types
//     *and* `web `-prefixed W3C custom formats (chromium internally calls
//     `GetStandardFormats` + `ExtractCustomPlatformNames` and merges).
//   * `GetAllAvailableFormats` — every other raw platform clipboard
//     format. Standard formats are filtered via `IsStandardClipboardFormat`
//     (they were already surfaced under a MIME above); the remainder is
//     wrapped as `electron application/osclipboard;format="..."`. The
//     macOS find pasteboard pseudo-MIME is decorated here.
//   * `ReadURL` (non-Linux, copy/paste buffer only) — adds
//     `electron application/bookmark` if a bookmark is present.
//
// The callbacks are built bottom-up because each step takes the next one
// as a bound argument, so the construction order below is the reverse of
// the execution order above.
void EnumerateAvailableTypes(ui::ClipboardBuffer buffer,
                             TypesCallback on_done) {
  // Bookmarks aren't reported by the format enumeration, so probe
  // `ReadURL` (non-Linux only — Linux has no bookmark concept on the
  // clipboard) to discover them before finalizing.
  TypesCallback add_bookmark = base::BindOnce(
      [](TypesCallback finalize, std::vector<std::string> types) {
#if !BUILDFLAG(IS_LINUX)
        ui::Clipboard::GetForCurrentThread()->ReadURL(
            /* data_dst = */ std::nullopt,
            base::BindOnce(
                [](TypesCallback finalize, std::vector<std::string> types,
                   ui::ClipboardUrlInfo url_info) {
                  if (!url_info.title.empty() || !url_info.url.is_empty()) {
                    types.emplace_back(
                        electron::api::clipboard_util::kBookmarkMime);
                  }
                  std::move(finalize).Run(std::move(types));
                },
                std::move(finalize), std::move(types)));
#else
        std::move(finalize).Run(std::move(types));
#endif
      },
      std::move(on_done));

  // Merge the remaining raw platform clipboard formats from
  // `GetAllAvailableFormats`. Standards (`IsStandardClipboardFormat`) are
  // filtered out since they were already surfaced under a MIME by
  // `ReadAvailableStandardAndCustomFormatNames`; everything else is
  // wrapped in the `osclipboard` MIME. The web-custom-format plumbing
  // slots (`org.w3.web-custom-format.map` / `type-N`) end up exposed too,
  // alongside their `web `-prefixed MIME twins — the platform names vary
  // by OS and surfacing them is harmless.
  TypesCallback merge_raw_formats = base::BindOnce(
      [](TypesCallback next, ui::ClipboardBuffer buf,
         std::vector<std::string> types) {
#if BUILDFLAG(IS_MAC)
        if (!electron::api::Clipboard::ReadFindText().empty()) {
          types.emplace_back(electron::api::clipboard_util::kFindTextMime);
        }
#endif
        ui::Clipboard::GetForCurrentThread()->GetAllAvailableFormats(
            buf, /* data_dst = */ std::nullopt,
            base::BindOnce(
                [](TypesCallback next, std::vector<std::string> types,
                   base::flat_set<ui::ClipboardFormatType> formats) {
                  for (const auto& fmt : formats) {
                    // Skip standards — already emitted as MIMEs above.
                    if (IsStandardClipboardFormat(fmt))
                      continue;
                    // On Windows `GetName()` yields the numeric registered-
                    // format id; resolve it back to the registered string
                    // name so the MIME round-trips on write and read.
                    const std::string name = electron::api::clipboard_util::
                        ResolvePlatformFormatName(fmt);
                    if (name.empty())
                      continue;
                    // A `web `-prefixed platform name (defensive — not
                    // normally produced) is exposed verbatim; everything
                    // else is wrapped in the osclipboard MIME.
                    std::string mime =
                        base::StartsWith(name, ui::kWebClipboardFormatPrefix)
                            ? name
                            : BuildOSClipboardFormat(name);
                    if (std::find(types.begin(), types.end(), mime) ==
                        types.end()) {
                      types.push_back(std::move(mime));
                    }
                  }
                  std::move(next).Run(std::move(types));
                },
                std::move(next), std::move(types)));
      },
      std::move(add_bookmark), buffer);

  // Kick the chain off with `ReadAvailableStandardAndCustomFormatNames`,
  // which returns standard MIMEs and `web `-prefixed W3C custom formats.
  // Chromium hands us a `vector<u16string>`; UTF-8 conversion happens once
  // here so the rest of the pipeline runs on `std::string`.
  ui::Clipboard::GetForCurrentThread()
      ->ReadAvailableStandardAndCustomFormatNames(
          buffer, /* data_dst = */ std::nullopt,
          base::BindOnce(
              [](TypesCallback next, std::vector<std::u16string> u16_types) {
                std::vector<std::string> types;
                types.reserve(u16_types.size());
                for (const auto& u16 : u16_types)
                  types.emplace_back(base::UTF16ToUTF8(u16));
                std::move(next).Run(std::move(types));
              },
              std::move(merge_raw_formats)));
}

}  // namespace

namespace electron::api {

namespace clipboard_util {

std::optional<std::string> ParseOSClipboardFormat(const std::string& mime) {
  // Expecting `<kOSClipboardMimePrefix>;format="<name>"`. Peel the prefix
  // and the trailing quote with `base::Remove{Prefix,Suffix}`; if anything
  // doesn't line up, return nullopt.
  auto after_prefix = base::RemovePrefix(mime, kOSClipboardMimePrefix);
  if (!after_prefix)
    return std::nullopt;
  auto after_eq = base::RemovePrefix(*after_prefix, ";format=\"");
  if (!after_eq)
    return std::nullopt;
  auto name = base::RemoveSuffix(*after_eq, "\"");
  if (!name)
    return std::nullopt;
  return std::string{*name};
}

#if !BUILDFLAG(IS_WIN)
// Windows needs a `GetClipboardFormatName` lookup — see
// `electron_api_clipboard_win.cc`. Everywhere else `GetName()` already
// returns the format's string name.
std::string ResolvePlatformFormatName(const ui::ClipboardFormatType& fmt) {
  return fmt.GetName();
}
#endif

}  // namespace clipboard_util

// Resolves `true` iff `format_string` appears in the same aggregated MIME
// list `clipboard.read()` exposes. This means `has` is consistent with
// `read` for every MIME the API surfaces: standard types, `web `-prefixed
// W3C custom formats, `electron application/osclipboard;format="..."`
// raw formats, `electron application/bookmark`, and the macOS find
// pasteboard pseudo-MIME.
v8::Local<v8::Promise> Clipboard::Has(ui::ClipboardBuffer buffer,
                                      const std::string& format_string,
                                      gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  EnumerateAvailableTypes(
      buffer, base::BindOnce(
                  [](gin_helper::Promise<bool> promise, std::string needle,
                     std::vector<std::string> types) {
                    promise.Resolve(std::find(types.begin(), types.end(),
                                              needle) != types.end());
                  },
                  std::move(promise), format_string));

  return handle;
}

v8::Local<v8::Promise> Clipboard::ReadText(ui::ClipboardBuffer buffer,
                                           gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Promise<std::u16string> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  ui::Clipboard::GetForCurrentThread()->ReadText(
      buffer, /* data_dst = */ std::nullopt,
      base::BindOnce(
          [](gin_helper::Promise<std::u16string> promise,
             ui::ClipboardBuffer buf, std::u16string result) {
#if BUILDFLAG(IS_WIN)
            if (result.empty()) {
              ui::Clipboard::GetForCurrentThread()->ReadAsciiText(
                  buf, /* data_dst = */ std::nullopt,
                  base::BindOnce(
                      [](gin_helper::Promise<std::u16string> p,
                         std::string ascii) {
                        p.Resolve(base::ASCIIToUTF16(ascii));
                      },
                      std::move(promise)));
              return;
            }
#endif
            promise.Resolve(std::move(result));
          },
          std::move(promise), buffer));

  return handle;
}

// Async W3C `clipboard.read()` — resolves to a one-element
// `ClipboardItem[]` whose `types` array is the full aggregated MIME list
// from `EnumerateAvailableTypes`. The `ClipboardItem`'s `getType(mime)`
// talks to `ui::Clipboard` on demand — no v8 closures are captured at
// `clipboard.read()` time.
v8::Local<v8::Promise> Clipboard::Read(ui::ClipboardBuffer buffer,
                                       gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  EnumerateAvailableTypes(
      buffer, base::BindOnce(
                  [](gin_helper::Promise<v8::Local<v8::Value>> promise,
                     ui::ClipboardBuffer buf, std::vector<std::string> types) {
                    v8::Isolate* iso = promise.isolate();
                    v8::HandleScope scope{iso};
                    v8::Local<v8::Context> ctx = iso->GetCurrentContext();

                    ClipboardItem* item = ClipboardItem::CreateForRead(
                        iso, buf, std::move(types));
                    v8::Local<v8::Value> wrapper;
                    if (!gin::TryConvertToV8(iso, item, &wrapper)) {
                      promise.RejectWithErrorMessage(
                          "Failed to create ClipboardItem wrapper");
                      return;
                    }
                    v8::Local<v8::Array> array = v8::Array::New(iso, 1);
                    array->Set(ctx, 0, wrapper).Check();
                    promise.Resolve(array);
                  },
                  std::move(promise), buffer));

  return handle;
}

// Atomic write of every MIME-keyed entry across the supplied
// `ClipboardItem`s. gin's vector converter (delegating to
// `Converter<cppgc::Persistent<ClipboardItem>>`) handles the
// array-of-`ClipboardItem` conversion automatically — non-array inputs
// and arrays containing non-`ClipboardItem` values are rejected by gin
// before this function runs, and per-MIME payload validation already
// happened in the `ClipboardItem` constructor. All `Write` has to do is
// reject read-side items and dispatch each item's `WriteTo` against a
// single `ScopedClipboardWriter`.
v8::Local<v8::Promise> Clipboard::Write(
    ui::ClipboardBuffer buffer,
    const std::vector<cppgc::Persistent<ClipboardItem>>& items,
    gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // ClipboardItems returned by `clipboard.read()` don't carry the user's
  // payload object — they're lightweight readers bound to a buffer and
  // can't be passed back to `write()`.
  for (const auto& item : items) {
    if (item->is_read_side()) {
      promise.Reject(v8::Exception::TypeError(gin::StringToV8(
          isolate,
          "clipboard.write cannot accept a ClipboardItem returned from "
          "clipboard.read() — construct a new ClipboardItem to write.")));
      return handle;
    }
  }

  // `ClipboardItem::WriteTo` dispatches each MIME to the right writer
  // method; the macOS find-pasteboard pseudo-MIME is committed directly
  // to its separate NSPasteboard from within `WriteTo`.
  {
    ui::ScopedClipboardWriter writer{buffer};
    for (const auto& item : items)
      item->WriteTo(writer);
    // `ScopedClipboardWriter`'s destructor runs here, committing every
    // accumulated entry to the system clipboard atomically.
  }

  promise.Resolve();
  return handle;
}

v8::Local<v8::Promise> Clipboard::WriteText(ui::ClipboardBuffer buffer,
                                            const std::u16string& text,
                                            gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  {
    ui::ScopedClipboardWriter writer{buffer};
    writer.WriteText(text);
  }

  promise.Resolve();
  return handle;
}

void Clipboard::Clear(ui::ClipboardBuffer buffer) {
  ui::Clipboard::GetForCurrentThread()->Clear(buffer);
}

#if !BUILDFLAG(IS_MAC)
// Mac impls live in `electron_api_clipboard_mac.mm`.
void Clipboard::WriteFindText(const std::u16string& text) {}
std::u16string Clipboard::ReadFindText() {
  return {};
}
#endif

// This exists for testing purposes ONLY.
void Clipboard::WriteFilesForTesting(const std::vector<base::FilePath>& files) {
  auto to_info = [](const auto& p) { return ui::FileInfo{p, p.BaseName()}; };
  auto file_infos = base::ToVector(files, to_info);
  ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
  writer.WriteFilenames(ui::FileInfosToURIList(file_infos));
}

}  // namespace electron::api

namespace {

// Bake `buffer` into each method and attach the resulting v8 functions to
// `target` so the JS-facing object has the public `Electron.Clipboard`
// shape (clear, has, read, readText, write, writeText). On Linux,
// `Initialize` calls this twice — once for the main copy/paste clipboard
// and once for the selection clipboard.
void PopulateClipboardObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context,
                             v8::Local<v8::Object> target,
                             ui::ClipboardBuffer buffer) {
  using electron::api::Clipboard;

  auto set = [&](std::string_view name, auto callback) {
    auto tmpl = gin::CreateFunctionTemplate(isolate, std::move(callback));
    target
        ->Set(context, gin::StringToV8(isolate, name),
              tmpl->GetFunction(context).ToLocalChecked())
        .Check();
  };

  set("clear", base::BindRepeating(&Clipboard::Clear, buffer));
  set("has", base::BindRepeating(&Clipboard::Has, buffer));
  set("read", base::BindRepeating(&Clipboard::Read, buffer));
  set("readText", base::BindRepeating(&Clipboard::ReadText, buffer));
  set("write", base::BindRepeating(&Clipboard::Write, buffer));
  set("writeText", base::BindRepeating(&Clipboard::WriteText, buffer));
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();

  PopulateClipboardObject(isolate, context, exports,
                          ui::ClipboardBuffer::kCopyPaste);

#if BUILDFLAG(IS_LINUX)
  auto selection = v8::Object::New(isolate);
  PopulateClipboardObject(isolate, context, selection,
                          ui::ClipboardBuffer::kSelection);
  exports->Set(context, gin::StringToV8(isolate, "selection"), selection)
      .Check();
#endif

  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod("_writeFilesForTesting",
                 &electron::api::Clipboard::WriteFilesForTesting);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_clipboard, Initialize)
