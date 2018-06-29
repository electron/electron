// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/dictionary.h"

namespace mate {

Dictionary::Dictionary() : isolate_(NULL) {}

Dictionary::Dictionary(v8::Isolate* isolate, v8::Local<v8::Object> object)
    : isolate_(isolate), object_(object) {}

Dictionary::Dictionary(const Dictionary& other) = default;

Dictionary::~Dictionary() = default;

Dictionary Dictionary::CreateEmpty(v8::Isolate* isolate) {
  return Dictionary(isolate, v8::Object::New(isolate));
}

v8::Local<v8::Object> Dictionary::GetHandle() const {
  return object_;
}

v8::Local<v8::Value> Converter<Dictionary>::ToV8(v8::Isolate* isolate,
                                                 Dictionary val) {
  return val.GetHandle();
}

bool Converter<Dictionary>::FromV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val,
                                   Dictionary* out) {
  if (!val->IsObject() || val->IsFunction())
    return false;
  *out = Dictionary(isolate, v8::Local<v8::Object>::Cast(val));
  return true;
}

}  // namespace mate
