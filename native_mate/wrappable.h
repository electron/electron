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
//   ...
// };
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a mate::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
class WrappableBase {
 public:
  WrappableBase();
  virtual ~WrappableBase();

  // Retrieve the v8 wrapper object cooresponding to this object.
  v8::Local<v8::Object> GetWrapper();

  // Returns the Isolate this object is created in.
  v8::Isolate* isolate() const { return isolate_; }

 protected:
  // Called after the "_init" method gets called in JavaScript.
  virtual void AfterInit(v8::Isolate* isolate) {}

  // Bind the C++ class to the JS wrapper.
  // This method should only be called by classes using Constructor.
  virtual void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

 private:
  friend struct internal::Destroyable;

  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);

  v8::Isolate* isolate_;
  v8::Global<v8::Object> wrapper_;  // Weak

  DISALLOW_COPY_AND_ASSIGN(WrappableBase);
};

template<typename T>
class Wrappable : public WrappableBase {
 public:
  Wrappable() {}

 protected:
  // Init the class with T::BuildPrototype.
  void Init(v8::Isolate* isolate) {
    // Fill the object template.
    if (!templ_) {
      v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate);
      T::BuildPrototype(isolate, templ->PrototypeTemplate());
      templ_ = new v8::Global<v8::FunctionTemplate>(isolate, templ);
    }

    v8::Local<v8::Object> wrapper;
    v8::Local<v8::FunctionTemplate> templ =
        v8::Local<v8::FunctionTemplate>::New(isolate, *templ_);
    // |wrapper| may be empty in some extreme cases, e.g., when
    // Object.prototype.constructor is overwritten.
    if (!templ->PrototypeTemplate()->NewInstance(
          isolate->GetCurrentContext()).ToLocal(&wrapper)) {
      // The current wrappable object will be no longer managed by V8. Delete
      // this now.
      delete this;
      return;
    }
    InitWith(isolate, wrapper);
  }

 private:
  static v8::Global<v8::FunctionTemplate>* templ_;  // Leaked on purpose

  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};

// static
template<typename T>
v8::Global<v8::FunctionTemplate>* Wrappable<T>::templ_ = nullptr;

// This converter handles any subclass of Wrappable.
template <typename T>
struct Converter<T*,
                 typename std::enable_if<
                     std::is_convertible<T*, WrappableBase*>::value>::type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val)
      return val->GetWrapper();
    else
      return v8::Null(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<WrappableBase*>(
        internal::FromV8Impl(isolate, val)));
    return *out != nullptr;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_H_
