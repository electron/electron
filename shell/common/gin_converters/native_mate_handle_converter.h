// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_NATIVE_MATE_HANDLE_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_NATIVE_MATE_HANDLE_CONVERTER_H_

#include "gin/converter.h"
#include "native_mate/handle.h"

namespace gin {

// TODO(zcbenz): Remove this converter after native_mate is removed.

template <typename T>
struct Converter<mate::Handle<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const mate::Handle<T>& in) {
    return mate::ConvertToV8(isolate, in);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     mate::Handle<T>* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_NATIVE_MATE_HANDLE_CONVERTER_H_
