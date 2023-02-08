// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_

#include "base/values.h"
#include "gin/converter.h"

namespace gin {

template <>
struct Converter<base::Value::Dict> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value::Dict* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value::Dict& val);
};

template <>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& val);
};

template <>
struct Converter<base::Value::List> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value::List* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value::List& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_VALUE_CONVERTER_H_
