// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/gurl_converter.h"

namespace gin {

template <>
struct Converter<GURL> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const GURL& in) {
    return mate::ConvertToV8(isolate, in);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     GURL* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_GIN_ADAPTER_H_
