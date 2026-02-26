// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_

#include "base/values.h"
#include "gin/converter.h"

namespace gin {

template <>
struct Converter<base::ValueView> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::ValueView val);
};

template <>
struct Converter<base::DictValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::DictValue& val) {
    return gin::ConvertToV8(isolate, base::ValueView{val});
  }
};

template <>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& val) {
    return gin::ConvertToV8(isolate, base::ValueView{val});
  }
};

template <>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::ListValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::ListValue& val) {
    return gin::ConvertToV8(isolate, base::ValueView{val});
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
