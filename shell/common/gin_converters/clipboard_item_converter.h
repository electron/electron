// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CLIPBOARD_ITEM_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CLIPBOARD_ITEM_CONVERTER_H_

#include "gin/converter.h"
#include "v8/include/cppgc/persistent.h"

namespace electron::api {
class ClipboardItem;
}  // namespace electron::api

namespace gin {

// gin already provides `Converter<T*>` for any `gin::Wrappable<T>` subclass
// (see `gin/wrappable.h`), but the blink-gc plugin rejects
// `std::vector<T*>` for cppgc-managed `T` — so the auto-vector converter
// can't be used directly with `ClipboardItem*`. This specialization wraps
// the raw pointer in `cppgc::Persistent<ClipboardItem>`, a proper GC
// root, which the plugin *does* allow inside `std::vector`. Combined
// with gin's built-in `Converter<std::vector<T>>`, this lets a function
// take `const std::vector<cppgc::Persistent<ClipboardItem>>&` directly
// — gin handles the array iteration and per-element validation.
template <>
struct Converter<cppgc::Persistent<electron::api::ClipboardItem>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     cppgc::Persistent<electron::api::ClipboardItem>* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CLIPBOARD_ITEM_CONVERTER_H_
