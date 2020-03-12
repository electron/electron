// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef SHELL_COMMON_GIN_HELPER_V8_TYPE_TRAITS_H_
#define SHELL_COMMON_GIN_HELPER_V8_TYPE_TRAITS_H_

#include <string>
#include <vector>

#include "gin/converter.h"

namespace gin_helper {

// Used by gin_helper::Arguments to get the JavaScript type name of a native
// C++ type when printing errors. All type names must be hard-coded.
//
// There a two ways of adding a type name:
//
// 1. Add a static "type_name" member in custom gin::Converter.
//    template <>
//    struct Converter<GURL> {
//      static constexpr const char* type_name = "url";
//      static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const GURL& val);
//    };
//
// 2. Explicitly define in this class. We should only do this way when adding
//    type name for preset converters in gin/converter.h.

template <typename T, typename Enable = void>
struct V8TypeTraits {
  static constexpr const bool has_type_name = false;
  static constexpr const char* type_name = "<unknown>";
};

// Read type information from converter.
template <typename T>
struct V8TypeTraits<T,
                    typename std::enable_if<std::is_pointer<decltype(
                        gin::Converter<T>::type_name)>::value>::type> {
  static constexpr const bool has_type_name = true;
  static constexpr const char* type_name = gin::Converter<T>::type_name;
};

// Provide type information for types in gin/converter.h.
#define DEFINE_TYPE_NAME(TYPE, NAME)                  \
  template <>                                         \
  struct V8TypeTraits<TYPE> {                         \
    static constexpr const bool has_type_name = true; \
    static constexpr const char* type_name = #NAME;   \
  }

DEFINE_TYPE_NAME(int32_t, Int32);
DEFINE_TYPE_NAME(uint32_t, UInt32);
DEFINE_TYPE_NAME(int64_t, Int64);
DEFINE_TYPE_NAME(uint64_t, UInt64);
DEFINE_TYPE_NAME(float, Number);
DEFINE_TYPE_NAME(double, Number);
DEFINE_TYPE_NAME(std::string, String);
DEFINE_TYPE_NAME(base::string16, String);
DEFINE_TYPE_NAME(v8::Local<v8::Function>, Function);
DEFINE_TYPE_NAME(v8::Local<v8::Object>, Object);
DEFINE_TYPE_NAME(v8::Local<v8::Promise>, Promise);
DEFINE_TYPE_NAME(v8::Local<v8::ArrayBuffer>, Buffer);

#undef DEFINE_TYPE_NAME

template <typename T>
struct V8TypeTraits<std::vector<T>> {
  static constexpr const bool has_type_name = true;
  static constexpr const char* type_name = "Array";
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_V8_TYPE_TRAITS_H_
