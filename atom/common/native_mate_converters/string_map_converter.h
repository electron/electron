// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_

#include <map>
#include <string>

#include "native_mate/converter.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<std::map<std::string, std::string>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::map<std::string, std::string>* out);

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
    const std::map<std::string, std::string>& in);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING_MAP_CONVERTER_H_
