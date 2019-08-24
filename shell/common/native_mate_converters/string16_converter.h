// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_

#include "gin/converter.h"
#include "native_mate/converter.h"

namespace mate {

template <>
struct Converter<base::string16> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::string16& val) {
    return gin::Converter<base::string16>::ToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::string16* out) {
    return gin::Converter<base::string16>::FromV8(isolate, val, out);
  }
};

inline v8::Local<v8::String> StringToV8(v8::Isolate* isolate,
                                        const base::string16& input) {
  return ConvertToV8(isolate, input).As<v8::String>();
}

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_
