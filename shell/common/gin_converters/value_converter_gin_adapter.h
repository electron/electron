// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/value_converter.h"

// TODO(zcbenz): Move the implementations from native_mate_converters to here.

namespace gin {

template <>
struct Converter<base::DictionaryValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictionaryValue* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::DictionaryValue& val) {
    return mate::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& in) {
    return mate::ConvertToV8(isolate, in);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_GIN_ADAPTER_H_
