// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_STRING16_CONVERSIONS_H_
#define COMMON_STRING16_CONVERSIONS_H_

#include "v8/include/v8.h"

class string16;

// Converts a V8 value to a string16.
inline string16 V8ValueToUTF16(v8::Handle<v8::Value> value) {
  v8::String::Value s(value);
  return string16(reinterpret_cast<const char16*>(*s), s.length());
}

// Converts string16 to V8 String.
inline v8::Handle<v8::Value> UTF16ToV8Value(const string16& s) {
  return v8::String::New(reinterpret_cast<const uint16_t*>(s.data()), s.size());
}

#endif  // COMMON_STRING16_CONVERSIONS_H_
