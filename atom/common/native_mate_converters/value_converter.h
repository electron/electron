// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "native_mate/converter.h"

namespace base {
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

}  // namespace mate
