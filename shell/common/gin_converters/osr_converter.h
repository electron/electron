// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/osr/osr_paint_event.h"
#include "ui/base/ime/text_input_mode.h"
#include "ui/base/ime/text_input_type.h"

namespace ui {
struct ImeTextSpan;
}

namespace gin {

template <>
struct Converter<electron::OffscreenSharedTextureValue> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::OffscreenSharedTextureValue& val);
};

template <>
struct Converter<ui::ImeTextSpan> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::ImeTextSpan* out);
};

template <>
struct Converter<ui::TextInputType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, ui::TextInputType val);
};

template <>
struct Converter<ui::TextInputMode> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, ui::TextInputMode val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_
