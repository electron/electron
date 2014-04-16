// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_WRAPPABLE_H_
#define NATIVE_MATE_WRAPPABLE_H_

#include "native_mate/compat.h"
#include "native_mate/converter.h"
#include "native_mate/template_util.h"

namespace mate {

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Handle<v8::Value> val);

}  // namespace internal


// WrappableBase is a base class for C++ objects that have corresponding v8 wrapper
// objects. To retain a WrappableBase object on the stack, use a mate::Handle.
//
// USAGE:
// // my_class.h
// class MyClass : WrappableBase {
//  public:
//   // Optional, only required if non-empty template should be used.
//   virtual mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
//       v8::Isolate* isolate);
//   ...
// };
//
// mate::ObjectTemplateBuilder MyClass::GetObjectTemplateBuilder(
//     v8::Isolate* isolate) {
//   return WrappableBase::GetObjectTemplateBuilder(isolate).SetValue("foobar", 42);
// }
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a mate::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
template<typename T>
class Wrappable;

class ObjectTemplateBuilder;

// Non-template base class to share code between templates instances.
class WrappableBase {
 protected:
  WrappableBase();
  virtual ~WrappableBase();

  virtual ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate);

  v8::Handle<v8::Object> GetWrapperImpl(v8::Isolate* isolate);

 private:
  static MATE_WEAK_CALLBACK(WeakCallback, v8::Object, WrappableBase);

  v8::Persistent<v8::Object> wrapper_;  // Weak

  DISALLOW_COPY_AND_ASSIGN(WrappableBase);
};


template<typename T>
class Wrappable : public WrappableBase {
 public:
  // Retrieve (or create) the v8 wrapper object cooresponding to this object.
  v8::Handle<v8::Object> GetWrapper(v8::Isolate* isolate) {
    return GetWrapperImpl(isolate);
  }

 protected:
  Wrappable() {}
  virtual ~Wrappable() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};


// This converter handles any subclass of WrappableBase.
template<typename T>
struct Converter<T*, typename enable_if<
                       is_convertible<T*, WrappableBase*>::value>::type> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    return val->GetWrapper(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<WrappableBase*>(
        internal::FromV8Impl(isolate, val)));
    return *out != NULL;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_H_
