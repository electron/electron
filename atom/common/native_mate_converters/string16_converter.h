// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_

#include "base/strings/string16.h"
#include "native_mate/converter.h"

namespace mate {

template<>
struct Converter<string16> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const string16& val) {
    return MATE_STRING_NEW_FROM_UTF16(
        isolate, reinterpret_cast<const uint16_t*>(val.data()), val.size());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     string16* out) {
    v8::String::Value s(val);
    out->assign(reinterpret_cast<const char16*>(*s), s.length());
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_STRING16_CONVERTER_H_
