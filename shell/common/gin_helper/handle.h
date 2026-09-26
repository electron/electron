// Copyright (c) 2025 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_

#include "base/memory/stack_allocated.h"
#include "gin/converter.h"
#include "v8/include/cppgc/type-traits.h"

namespace gin_helper {

// Use gin_helper::Handle on the stack to retain a non-cppgc
// gin_helper::Wrappable object and its V8 wrapper together.
//
// This class must NOT be used with cppgc-managed types (gin::Wrappable).
// For cppgc types, use T* directly and gin::Converter<T*> for V8 conversion.
template <typename T>
class Handle {
  static_assert(!cppgc::IsGarbageCollectedTypeV<T>,
                "gin_helper::Handle must not be used with cppgc "
                "garbage-collected types. Use T* directly instead.");
  STACK_ALLOCATED();

 public:
  Handle() : object_(nullptr) {}

  Handle(v8::Local<v8::Value> wrapper, T* object)
      : wrapper_(wrapper), object_(object) {}

  bool IsEmpty() const { return !object_; }

  void Clear() {
    wrapper_.Clear();
    object_ = nullptr;
  }

  T* operator->() const { return object_; }
  v8::Local<v8::Value> ToV8() const { return wrapper_; }
  T* get() const { return object_; }

 private:
  v8::Local<v8::Value> wrapper_;
  T* object_;
};

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
    T* object = nullptr;
    if (!Converter<T*>::FromV8(isolate, val, &object)) {
      return false;
    }
    *out = gin_helper::Handle<T>(val, object);
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_HANDLE_H_
