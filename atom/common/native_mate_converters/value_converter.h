// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_

#include "native_mate/converter.h"

namespace base {
class BinaryValue;
class DictionaryValue;
class ListValue;
}

namespace mate {

template<>
struct Converter<base::DictionaryValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictionaryValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::DictionaryValue& val);
};

template<>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::ListValue* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::ListValue& val);
};

template<>
struct Converter<base::BinaryValue> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::BinaryValue& val);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_VALUE_CONVERTER_H_
