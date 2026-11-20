// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_

#include <string>

#include "gin/converter.h"

namespace ui {
class Accelerator;
}

namespace electron {

// A global shortcut as accepted by the `globalShortcut` module: either a bare
// accelerator string, or an object that also carries a user-facing description.
struct ShortcutRegistration {
  // The accelerator, exactly as the app passed it. Reported verbatim by the
  // `registration-resolved` event.
  std::string accelerator;
  // User-readable description of what the shortcut does. Empty means "use the
  // default description".
  std::u16string description;
};

}  // namespace electron

namespace gin {

template <>
struct Converter<ui::Accelerator> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::Accelerator* out);
};

template <>
struct Converter<electron::ShortcutRegistration> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::ShortcutRegistration* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ACCELERATOR_CONVERTER_H_
