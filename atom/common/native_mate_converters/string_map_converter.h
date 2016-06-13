// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_

#include <map>
#include <string>

#include "native_mate/converter.h"

namespace mate {

template<>
struct Converter<std::map<std::string, std::string> > {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::map<std::string, std::string>* out) {
    if (!val->IsObject())
      return false;

    v8::Local<v8::Object> dict = val->ToObject();
    v8::Local<v8::Array> keys = dict->GetOwnPropertyNames();
    for (uint32_t i = 0; i < keys->Length(); ++i) {
      v8::Local<v8::Value> key = keys->Get(i);
      (*out)[V8ToString(key)] = V8ToString(dict->Get(key));
    }
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_
