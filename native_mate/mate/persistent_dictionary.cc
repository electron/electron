// Copyright 2014 Cheng Zhao. All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#include "native_mate/persistent_dictionary.h"

namespace mate {

PersistentDictionary::PersistentDictionary() {
}

PersistentDictionary::PersistentDictionary(v8::Isolate* isolate,
                                           v8::Local<v8::Object> object)
    : handle_(new RefCountedPersistent<v8::Object>(isolate, object)) {
  isolate_ = isolate;
}

PersistentDictionary::~PersistentDictionary() {
}

v8::Local<v8::Object> PersistentDictionary::GetHandle() const {
  return handle_->NewHandle();
}

bool Converter<PersistentDictionary>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             PersistentDictionary* out) {
  if (!val->IsObject())
    return false;
  *out = PersistentDictionary(isolate, v8::Local<v8::Object>::Cast(val));
  return true;
}

}  // namespace mate
