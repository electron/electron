// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_HANDLE_H_
#define NATIVE_MATE_HANDLE_H_

#include "native_mate/converter.h"

namespace mate {

// You can use mate::Handle on the stack to retain a mate::Wrappable object.
// Currently we don't have a mechanism for retaining a mate::Wrappable object
// in the C++ heap because strong references from C++ to V8 can cause memory
// leaks.
template<typename T>
class Handle {
 public:
  Handle() : object_(NULL) {}

  Handle(v8::Local<v8::Object> wrapper, T* object)
    : wrapper_(wrapper),
      object_(object) {
  }

  bool IsEmpty() const { return !object_; }

  void Clear() {
    wrapper_.Clear();
    object_ = NULL;
  }

  T* operator->() const { return object_; }
  v8::Local<v8::Object> ToV8() const { return wrapper_; }
  T* get() const { return object_; }

 private:
  v8::Local<v8::Object> wrapper_;
  T* object_;
};

template<typename T>
struct Converter<mate::Handle<T> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const mate::Handle<T>& val) {
    return val.ToV8();
  }
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     mate::Handle<T>* out) {
    T* object = NULL;
    if (!Converter<T*>::FromV8(isolate, val, &object)) {
      return false;
    }
    *out = mate::Handle<T>(val->ToObject(), object);
    return true;
  }
};

// This function is a convenient way to create a handle from a raw pointer
// without having to write out the type of the object explicitly.
template<typename T>
mate::Handle<T> CreateHandle(v8::Isolate* isolate, T* object) {
  return mate::Handle<T>(object->GetWrapper(), object);
}

}  // namespace mate

#endif  // NATIVE_MATE_HANDLE_H_
