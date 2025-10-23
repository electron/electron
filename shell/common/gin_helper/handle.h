// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_

#include "base/memory/raw_ptr.h"
#include "gin/converter.h"

namespace gin_helper {

// You can use gin_helper::Handle on the stack to retain a gin_helper::Wrappable
// object. Currently we don't have a mechanism for retaining a
// gin_helper::Wrappable object in the C++ heap because strong references from
// C++ to V8 can cause memory leaks. Copied from
// https://chromium-review.googlesource.com/c/chromium/src/+/6734440 Should be
// removed once https://github.com/electron/electron/issues/47922 is complete.
template <typename T>
class Handle {
 public:
  Handle() : object_(nullptr) {}

  Handle(v8::Local<v8::Value> wrapper, T* object)
      : wrapper_(wrapper), object_(object) {}

  bool IsEmpty() const { return !object_; }

  void Clear() {
    wrapper_.Clear();
    object_ = NULL;
  }

  T* operator->() const { return object_; }
  v8::Local<v8::Value> ToV8() const { return wrapper_; }
  T* get() const { return object_; }

 private:
  v8::Local<v8::Value> wrapper_;
  raw_ptr<T> object_;
};

// This function is a convenient way to create a handle from a raw pointer
// without having to write out the type of the object explicitly.
template <typename T>
gin_helper::Handle<T> CreateHandle(v8::Isolate* isolate, T* object) {
  v8::Local<v8::Object> wrapper;
  if (!object->GetWrapper(isolate).ToLocal(&wrapper) || wrapper.IsEmpty())
    return gin_helper::Handle<T>();
  return gin_helper::Handle<T>(wrapper, object);
}

}  // namespace gin_helper

namespace gin {

template <typename T>
struct Converter<gin_helper::Handle<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gin_helper::Handle<T>& val) {
    return val.ToV8();
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::Handle<T>* out) {
    T* object = NULL;
    if (!Converter<T*>::FromV8(isolate, val, &object)) {
      return false;
    }
    *out = gin_helper::Handle<T>(val, object);
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_
