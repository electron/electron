// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/common/gin_converters/native_window_converter.h"

namespace mate {

template <>
struct Converter<electron::NativeWindow*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::NativeWindow** out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_NATIVE_WINDOW_CONVERTER_H_
