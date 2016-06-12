// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "base/strings/string16.h"
#include "native_mate/converter.h"

namespace mate {

template<>
struct Converter<base::string16> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const base::string16& val) {
    return MATE_STRING_NEW_FROM_UTF16(
        isolate, reinterpret_cast<const uint16_t*>(val.data()), val.size());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::string16* out) {
    if (!val->IsString())
      return false;

    v8::String::Value s(val);
    out->assign(reinterpret_cast<const base::char16*>(*s), s.length());
    return true;
  }
};

inline v8::Local<v8::String> StringToV8(
    v8::Isolate* isolate,
    const base::string16& input) {
  return ConvertToV8(isolate, input).As<v8::String>();
}

}  // namespace mate
