// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ABSL_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ABSL_CONVERTER_H_

#include "gin/converter.h"

namespace ui {
class Accelerator;
}

namespace gin {

template <typename T>
struct Converter<absl::optional<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   absl::optional<T> status) {
    if (status) {
      return gin::ConvertToV8(isolate, *status);
    }
    return v8::Undefined(isolate);
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_ABSL_CONVERTER_H_
