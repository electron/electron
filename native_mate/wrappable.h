// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_WRAPPABLE_H_
#define NATIVE_MATE_WRAPPABLE_H_

#include "base/template_util.h"
#include "native_mate/compat.h"
#include "native_mate/converter.h"

namespace mate {

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Handle<v8::Value> val);

}  // namespace internal


// Wrappable is a base class for C++ objects that have corresponding v8 wrapper
// objects. To retain a Wrappable object on the stack, use a mate::Handle.
//
// USAGE:
// // my_class.h
// class MyClass : Wrappable {
//  public:
//   // Optional, only required if non-empty template should be used.
//   virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
//       v8::Isolate* isolate);
//   ...
// };
//
// mate::ObjectTemplateBuilder MyClass::GetObjectTemplateBuilder(
//     v8::Isolate* isolate) {
//   return Wrappable::GetObjectTemplateBuilder(isolate).SetValue("foobar", 42);
// }
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a mate::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
class ObjectTemplateBuilder;

// Non-template base class to share code between templates instances.
class Wrappable {
 public:
  // Retrieve (or create) the v8 wrapper object cooresponding to this object.
  v8::Handle<v8::Object> GetWrapper(v8::Isolate* isolate);

 protected:
  Wrappable();
  virtual ~Wrappable();

  virtual ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate);

 private:
  static MATE_WEAK_CALLBACK(WeakCallback, v8::Object, Wrappable);

  v8::Persistent<v8::Object> wrapper_;  // Weak

  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};


// This converter handles any subclass of Wrappable.
template<typename T>
struct Converter<T*, typename base::enable_if<
                       base::is_convertible<T*, Wrappable*>::value>::type> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    return val->GetWrapper(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<Wrappable*>(
        internal::FromV8Impl(isolate, val)));
    return *out != NULL;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_H_
