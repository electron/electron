
// Copyright (c) 2024 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_

#include "gin/converter.h"
#include "shell/browser/osr/osr_paint_event.h"

namespace gin {

template <>
struct Converter<electron::OffscreenSharedTextureValue> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const electron::OffscreenSharedTextureValue& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_OSR_CONVERTER_H_
