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

struct Destroyable;

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val);

}  // namespace internal


// Wrappable is a base class for C++ objects that have corresponding v8 wrapper
// objects. To retain a Wrappable object on the stack, use a gin::Handle.
//
// USAGE:
// // my_class.h
// class MyClass : Wrappable<MyClass> {
//  public:
//   // Optional, only required if non-empty template should be used.
//   virtual gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
//       v8::Isolate* isolate);
//   ...
// };
//
// // my_class.cc
// gin::ObjectTemplateBuilder MyClass::GetObjectTemplateBuilder(
//     v8::Isolate* isolate) {
//   return Wrappable<MyClass>::GetObjectTemplateBuilder(isolate)
//       .SetValue("foobar", 42);
// }
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a mate::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
class ObjectTemplateBuilder;

class Wrappable {
 public:
  // Retrieve (or create) the v8 wrapper object cooresponding to this object.
  // If the type is created via the Constructor, then the GetWrapper would
  // return the constructed object, otherwise it would try to create a new
  // object constructed by GetObjectTemplateBuilder.
  v8::Local<v8::Object> GetWrapper(v8::Isolate* isolate);

  // Returns the Isolate this object is created in.
  v8::Isolate* isolate() const { return isolate_; }

  // Bind the C++ class to the JS wrapper.
  void Wrap(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  // The user should define T::BuildPrototype if they want to use Constructor
  // to build a constructor function for this type.
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  Wrappable();
  virtual ~Wrappable();

  virtual ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate);

  // Called after the "_init" method gets called in JavaScript.
  virtual void AfterInit(v8::Isolate* isolate) {}

 private:
  friend struct internal::Destroyable;

  static void FirstWeakCallback(const v8::WeakCallbackInfo<Wrappable>& data);
  static void SecondWeakCallback(const v8::WeakCallbackInfo<Wrappable>& data);

  v8::Isolate* isolate_;
  v8::UniquePersistent<v8::Object> wrapper_;  // Weak

  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};


// This converter handles any subclass of Wrappable.
template<typename T>
struct Converter<T*, typename enable_if<
                       is_convertible<T*, Wrappable*>::value>::type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val)
      return val->GetWrapper(isolate);
    else
      return v8::Null(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<Wrappable*>(
        internal::FromV8Impl(isolate, val)));
    return *out != NULL;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_H_
