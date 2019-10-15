// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_

#include <map>

#include "shell/common/gin_converters/std_converter.h"

namespace mate {

template <typename K, typename V>
struct Converter<std::map<K, V>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::map<K, V>* out) {
    return gin::Converter<std::map<K, V>>::FromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::map<K, V>& val) {
    return gin::Converter<std::map<K, V>>::ToV8(isolate, val);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_
