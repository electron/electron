// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CLIPBOARD_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CLIPBOARD_H_

#include <optional>
#include <string>
#include <string_view>

#include "ui/base/clipboard/clipboard.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-forward.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace electron::api {

class ClipboardItem;

// Shared MIME-type constants and helpers used by both the `Clipboard`
// static methods (`Has`, `Read`, `Write`) and the `ClipboardItem` class
// (`ClipboardItem::GetType`). They live here rather than in either `.cc`'s
// anonymous namespace because both translation units need them and a
// shared header avoids the alternative of duplicating the same strings.
namespace clipboard_util {

// Electron-specific MIME types and prefixes. The cross-platform standard
// MIME constants (`text/plain`, `text/html`, `text/rtf`, `image/png`,
// `web ` prefix, …) live in chromium's
// `ui/base/clipboard/clipboard_constants.h` (e.g. `ui::kMimeTypePlainText`,
// `ui::kWebClipboardFormatPrefix`) — use those directly rather than
// re-declaring them here.
inline constexpr std::string_view kOSClipboardMimePrefix =
    "electron application/osclipboard";
inline constexpr std::string_view kBookmarkMime =
    "electron application/bookmark";
inline constexpr std::string_view kFindTextMime =
    "electron application/findtext";
// Used to match any `image/*` MIME — there's no chromium constant for the
// bare prefix.
inline constexpr std::string_view kImagePrefix = "image/";

// Parse `electron application/osclipboard;format="<name>"` → "<name>".
std::optional<std::string> ParseOSClipboardFormat(const std::string& mime);

// Resolve a raw platform clipboard format to the stable string name used
// in the `electron application/osclipboard;format="<name>"` MIME. On
// Windows, `ClipboardFormatType::GetName()` returns the numeric
// registered-format id (e.g. `"49472"`); this resolves it back to the
// registered string name (e.g. `"public/utf8-plain-text"`) via
// `GetClipboardFormatName` so a raw format round-trips through the same
// MIME on write and read. On other platforms `GetName()` already returns
// the format's string name, so this is a thin pass-through.
std::string ResolvePlatformFormatName(const ui::ClipboardFormatType& fmt);

}  // namespace clipboard_util

// The `electron_browser_clipboard` native binding is shaped directly like
// the public `Electron.Clipboard` interface: it exposes `read`, `write`,
// `readText`, `writeText`, `has`, and `clear` methods plus an optional
// `selection` sub-object on Linux. Each method takes a `ui::ClipboardBuffer`
// as its first parameter — at binding-registration time the `Initialize`
// function uses `base::BindRepeating` + `gin::CreateFunctionTemplate` to
// bake the appropriate buffer into the v8 function for both the main
// clipboard (kCopyPaste) and the Linux selection clipboard (kSelection).
//
// All MIME-type dispatch (text/html/rtf/image/bookmark/findtext/web-custom-
// format/etc.) lives in this binding so the JavaScript-side `clipboard`
// module is just a re-export of `process._linkedBinding(...)`.
class Clipboard {
 public:
  // disable copy
  Clipboard(const Clipboard&) = delete;
  Clipboard& operator=(const Clipboard&) = delete;

  // Async; backed by Chromium's `Clipboard::GetAllAvailableFormats`.
  static v8::Local<v8::Promise> Has(ui::ClipboardBuffer buffer,
                                    const std::string& format_string,
                                    v8::Isolate* isolate);

  // Synchronous in Chromium (`ui::Clipboard::Clear`).
  static void Clear(ui::ClipboardBuffer buffer);

  // W3C-style read. Resolves to a one-element `ClipboardItem[]` whose
  // `types` array is the augmented MIME-type list (`ReadAvailableTypes` +
  // bookmark/findtext decoration). Per-type payloads are fetched lazily by
  // `ClipboardItem::GetType`, which is where the per-MIME read dispatch
  // table now lives.
  static v8::Local<v8::Promise> Read(ui::ClipboardBuffer buffer,
                                     v8::Isolate* isolate);

  // Fast path for the common `clipboard.readText()` call so it avoids the
  // per-MIME dispatch overhead used by `ClipboardItem::GetType`.
  static v8::Local<v8::Promise> ReadText(ui::ClipboardBuffer buffer,
                                         v8::Isolate* isolate);

  // Atomic write of every MIME-keyed entry across the supplied
  // `ClipboardItem`s. Returns a
  // Promise so the JS-facing shape matches W3C `clipboard.write`.
  static v8::Local<v8::Promise> Write(ui::ClipboardBuffer buffer,
                                      const v8::LocalVector<v8::Value>& items,
                                      v8::Isolate* isolate);

  // Convenience wrapper that funnels through `Write` so the public
  // `writeText` API uses the same atomic-write path.
  static v8::Local<v8::Promise> WriteText(ui::ClipboardBuffer buffer,
                                          const std::u16string& text,
                                          v8::Isolate* isolate);

  // The macOS find pasteboard is a separate pasteboard and the backing
  // Cocoa API is synchronous, so these stay synchronous as well.
  static std::u16string ReadFindText();
  static void WriteFindText(const std::u16string& text);
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_CLIPBOARD_H_
