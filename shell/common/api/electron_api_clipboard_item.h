// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_ELECTRON_API_CLIPBOARD_ITEM_H_
#define ELECTRON_SHELL_COMMON_API_ELECTRON_API_CLIPBOARD_ITEM_H_

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "base/containers/flat_map.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/constructible.h"
#include "ui/base/clipboard/clipboard_buffer.h"
#include "v8/include/v8-forward.h"

namespace cppgc {
class Visitor;
}  // namespace cppgc

namespace gin {
class Arguments;
}  // namespace gin

namespace ui {
class ScopedClipboardWriter;
}  // namespace ui

namespace electron::api {

// Structured payload for the `electron application/bookmark` MIME — a
// pre-parsed `{ title, url }` object the W3C clipboard write path
// commits via `ScopedClipboardWriter::WriteURL`.
struct BookmarkInfo {
  std::u16string title;
  std::string url;
};

// One write-side payload value, pre-parsed from JS by the `ClipboardItem`
// constructor into the C++ form `WriteTo` expects:
//   * `std::vector<uint8_t>` — raw bytes copied from a `Buffer` (used for
//     `web `, `osclipboard;format="..."`, `image/*`, and arbitrary
//     binary MIMEs).
//   * `std::u16string` — text from a JS string (used for text-typed
//     MIMEs: `text/plain`, `text/html`, `text/rtf`,
//     `electron application/findtext`).
//   * `BookmarkInfo` — the structured `{title, url}` form for
//     `electron application/bookmark`.
using ClipboardItemPayload =
    std::variant<std::vector<uint8_t>, std::u16string, BookmarkInfo>;

// `ClipboardItem` is the W3C-style clipboard entry exposed to JS. It is
// constructable from JS as `new ClipboardItem(items, options?)` and is also
// the type yielded by `clipboard.read()`. Two construction paths share the
// same wrapper class:
//
//   * Constructed in JS — the constructor parses the user's
//     `{ [mime]: payload }` object up front, validates each value against
//     the type the MIME expects (Buffer / string / `{title, url}` for the
//     bookmark MIME), and stores the result in `payloads_`. Type
//     mismatches throw a `TypeError` synchronously. `WriteTo` then walks
//     `payloads_` and dispatches each entry to the right
//     `ScopedClipboardWriter` method — `Clipboard::Write` itself just
//     iterates the items array, validates each is a non-read-side
//     `ClipboardItem`, and calls `WriteTo`.
//   * Built by `clipboard.read()` — stores just the available MIME types
//     and the source `ui::ClipboardBuffer`. `getType(mime)` reads from
//     the platform clipboard on demand via the per-MIME dispatch in this
//     file's anonymous namespace. Read-side items hold no payloads, no
//     bound v8 functions, and no captured callbacks.
//
// The class is cppgc-managed (via `gin::Wrappable`), so instances must be
// allocated with `cppgc::MakeGarbageCollected` and are reclaimed when the
// JS wrapper becomes unreachable.
class ClipboardItem final : public gin::Wrappable<ClipboardItem>,
                            public gin_helper::Constructible<ClipboardItem> {
 public:
  // disable copy
  ClipboardItem(const ClipboardItem&) = delete;
  ClipboardItem& operator=(const ClipboardItem&) = delete;

  // gin_helper::Constructible — invoked from JS via `new ClipboardItem(...)`.
  static ClipboardItem* New(gin::Arguments* args);
  static void FillObjectTemplate(v8::Isolate*, v8::Local<v8::ObjectTemplate>);
  static const char* GetClassName() { return "ClipboardItem"; }

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;

  // Factory for read-side items. `clipboard.read()` calls this to wrap the
  // enumerated MIME types and the source buffer into a `ClipboardItem`
  // instance that can lazily fetch payloads.
  static ClipboardItem* CreateForRead(v8::Isolate* isolate,
                                      ui::ClipboardBuffer buffer,
                                      std::vector<std::string> types);

  // Commit every MIME-keyed entry in `payloads_` to `writer`. The macOS
  // find-pasteboard pseudo-MIME is committed directly to the find
  // pasteboard via `Clipboard::WriteFindText` (a separate NSPasteboard
  // from the system clipboard, so it doesn't need to share the
  // `ScopedClipboardWriter`'s atomic-commit lifetime). No-op for
  // read-side items.
  void WriteTo(ui::ScopedClipboardWriter& writer) const;

  bool is_read_side() const { return is_read_side_; }
  const std::vector<std::string>& types() const { return types_; }

  // Public for `cppgc::MakeGarbageCollected`.
  // Read-side constructor.
  ClipboardItem(v8::Isolate* isolate,
                ui::ClipboardBuffer buffer,
                std::vector<std::string> types);
  // JS-constructed constructor. Pulls the `items` argument from `args`
  // (the `{ [mime]: payload }` object the W3C `ClipboardItem(items,
  // options?)` constructor takes), validates and parses each MIME's
  // value into a `ClipboardItemPayload`, and populates `types_` /
  // `payloads_`. Throws a `TypeError` (via `args->ThrowTypeError` or
  // `isolate->ThrowException`) if `args` is malformed or any payload
  // value's type doesn't match what the MIME expects; `ClipboardItem::New`
  // checks the pending exception after `MakeGarbageCollected` returns.
  explicit ClipboardItem(gin::Arguments* args);
  ~ClipboardItem() override;

 private:
  // Exposed to JS via the prototype template.
  std::vector<std::string> GetTypes() const { return types_; }
  v8::Local<v8::Promise> GetType(const std::string& mime, v8::Isolate* isolate);

  std::vector<std::string> types_;
  const bool is_read_side_;
  // Only meaningful when `is_read_side_` is true.
  const ui::ClipboardBuffer buffer_;
  // Only populated when `is_read_side_` is false. Pre-parsed payload for
  // each MIME, keyed by MIME name (matches `types_`).
  base::flat_map<std::string, ClipboardItemPayload> payloads_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_COMMON_API_ELECTRON_API_CLIPBOARD_ITEM_H_
