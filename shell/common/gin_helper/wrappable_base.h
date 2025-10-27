// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_

#include "base/memory/raw_ptr.h"
#include "v8/include/v8-forward.h"

namespace gin {
class Arguments;
class ObjectTemplateBuilder;
struct DeprecatedWrapperInfo;
}  // namespace gin

namespace gin_helper {

// Wrappable is a base class for C++ objects that have corresponding v8 wrapper
// objects. To retain a Wrappable object on the stack, use a gin_helper::Handle.
//
// USAGE:
// // my_class.h
// class MyClass : Wrappable<MyClass> {
//  public:
//   ...
// };
//
// Subclasses should also typically have private constructors and expose a
// static Create function that returns a gin_helper::Handle. Forcing creators
// through this static Create function will enforce that clients actually create
// a wrapper for the object. If clients fail to create a wrapper for a wrappable
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

  // Returns the Isolate this object is created in.
  v8::Isolate* isolate() const { return isolate_; }

 protected:
  // Bind the C++ class to the JS wrapper.
  // This method should only be called by classes using Constructor.
  virtual void InitWith(v8::Isolate* isolate, v8::Local<v8::Object> wrapper);

  // Helper to init with arguments.
  void InitWithArgs(const gin::Arguments* args);

  v8::Global<v8::Object> wrapper_;  // Weak

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WrappableBase>& data);

  raw_ptr<v8::Isolate> isolate_ = nullptr;
};

// Copied from https://chromium-review.googlesource.com/c/chromium/src/+/6799157
// Will be removed as part of https://github.com/electron/electron/issues/47922
class DeprecatedWrappableBase {
 public:
  DeprecatedWrappableBase(const DeprecatedWrappableBase&) = delete;
  DeprecatedWrappableBase& operator=(const DeprecatedWrappableBase&) = delete;

 protected:
  DeprecatedWrappableBase();
  virtual ~DeprecatedWrappableBase();

  // Overrides of this method should be declared final and not overridden again.
  virtual gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate);

  // Returns a readable type name that will be used in surfacing errors. The
  // default implementation returns nullptr, which results in a generic error.
  virtual const char* GetTypeName();

  v8::MaybeLocal<v8::Object> GetWrapperImpl(
      v8::Isolate* isolate,
      gin::DeprecatedWrapperInfo* wrapper_info);

  // Make this wrappable strong again. This is useful when the wrappable is
  // destroyed outside the finalizer callbacks and we want to avoid scheduling
  // the weak callbacks if they haven't been scheduled yet.
  // NOTE!!! this does not prevent finalization callbacks from running if they
  // have already been processed.
  void ClearWeak();

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<DeprecatedWrappableBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<DeprecatedWrappableBase>& data);

  bool dead_ = false;
  v8::Global<v8::Object> wrapper_;  // Weak
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_BASE_H_
