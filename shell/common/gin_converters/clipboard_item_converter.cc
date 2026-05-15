// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/clipboard_item_converter.h"

#include "shell/common/api/electron_api_clipboard_item.h"

namespace gin {

bool Converter<cppgc::Persistent<electron::api::ClipboardItem>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    cppgc::Persistent<electron::api::ClipboardItem>* out) {
  // Delegate the "is this a ClipboardItem wrapper?" check to gin's
  // built-in `Converter<ClipboardItem*>` (defined for any subclass of
  // `gin::Wrappable` in `gin/wrappable.h`), then wrap the raw pointer
  // in a `cppgc::Persistent` so the caller can keep it in a
  // `std::vector` without tripping the blink-gc plugin.
  electron::api::ClipboardItem* item = nullptr;
  if (!Converter<electron::api::ClipboardItem*>::FromV8(isolate, val, &item) ||
      item == nullptr) {
    return false;
  }
  *out = item;
  return true;
}

}  // namespace gin
