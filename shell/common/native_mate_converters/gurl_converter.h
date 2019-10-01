// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_GURL_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_GURL_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/common/gin_converters/gurl_converter.h"

namespace mate {

template <>
struct Converter<GURL> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const GURL& val) {
    return gin::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     GURL* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_GURL_CONVERTER_H_
