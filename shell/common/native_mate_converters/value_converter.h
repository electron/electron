// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_

#include "native_mate/converter.h"

#include "base/optional.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}  // namespace base

namespace mate {

template <>
struct Converter<base::DictionaryValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictionaryValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::DictionaryValue& val);
};

template <>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& val);
};

template <typename T>
struct Converter<base::Optional<T>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Optional<T>* out) {
    if (val->IsNull() || val->IsUndefined()) {
      *out = base::nullopt;
      return true;
    }
    T converted;
    if (!Converter<T>::FromV8(isolate, val, &converted)) {
      *out = base::nullopt;
      return true;
    }
    out->emplace(converted);
    return true;
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Optional<T>& val) {
    if (val.has_value())
      return Converter<T>::ToV8(val.value());
    return v8::Undefined(isolate);
  }
};

template <>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::ListValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::ListValue& val);
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_
