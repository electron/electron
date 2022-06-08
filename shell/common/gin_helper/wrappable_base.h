// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_

#include "v8/include/v8.h"

namespace gin {
class Arguments;
}

namespace gin_helper {

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
// static Create function that returns a gin::Handle. Forcing creators through
// this static Create function will enforce that clients actually create a
// wrapper for the object. If clients fail to create a wrapper for a wrappable
// object, the object will leak because we use the weak callback from the
// wrapper as the signal to delete the wrapped object.
class WrappableBase {
 public:
  WrappableBase();
  WrappableBase(const WrappableBase&) = delete;
  WrappableBase& operator=(const WrappableBase&) = delete;
  virtual ~WrappableBase();

  // Retrieve the v8 wrapper object corresponding to this object.
  v8::Local<v8::Object> GetWrapper() const;
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const;

  // Returns the Isolate this object is created in.
  v8::Isolate* isolate() const { return isolate_; }

 protected:
  // Called after the "_init" method gets called in JavaScript.
  virtual void AfterInit(v8::Isolate* isolate) {}

  // Bind the C++ class to the JS wrapper.
  // This method should only be called by classes using Constructor.
  virtual void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  // Helper to init with arguments.
  void InitWithArgs(gin::Arguments* args);

  v8::Global<v8::Object> wrapper_;  // Weak

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);

  v8::Isolate* isolate_ = nullptr;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
