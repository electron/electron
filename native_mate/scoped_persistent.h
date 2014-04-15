// Copyright 2014 Cheng Zhao. All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#ifndef NATIVE_MATE_SCOPED_PERSISTENT_H_
#define NATIVE_MATE_SCOPED_PERSISTENT_H_

#include "base/memory/ref_counted.h"
#include "v8/include/v8.h"

namespace mate {

// A v8::Persistent handle to a V8 value which destroys and clears the
// underlying handle on destruction.
template <typename T>
class ScopedPersistent {
 public:
  ScopedPersistent() {
  }

  explicit ScopedPersistent(v8::Handle<T> handle) {
    reset(handle);
  }

  ~ScopedPersistent() {
    reset();
  }

  void reset(v8::Handle<T> handle) {
    if (!handle.IsEmpty())
      handle_.Reset(GetIsolate(handle), handle);
    else
      reset();
  }

  void reset() {
    handle_.Reset();
  }

  bool IsEmpty() const {
    return handle_.IsEmpty();
  }

  v8::Handle<T> NewHandle() const {
    if (handle_.IsEmpty())
      return v8::Local<T>();
    return v8::Local<T>::New(GetIsolate(handle_), handle_);
  }

  v8::Handle<T> NewHandle(v8::Isolate* isolate) const {
    if (handle_.IsEmpty())
      return v8::Local<T>();
    return v8::Local<T>::New(isolate, handle_);
  }

  template <typename P>
  void MakeWeak(P* parameters,
                typename v8::WeakReferenceCallbacks<T, P>::Revivable callback) {
    handle_.MakeWeak(parameters, callback);
  }

 private:
  template <typename U>
  static v8::Isolate* GetIsolate(v8::Handle<U> object_handle) {
    // Only works for v8::Object and its subclasses. Add specialisations for
    // anything else.
    if (!object_handle.IsEmpty())
      return GetIsolate(object_handle->CreationContext());
    return v8::Isolate::GetCurrent();
  }
  static v8::Isolate* GetIsolate(v8::Handle<v8::Context> context_handle) {
    if (!context_handle.IsEmpty())
      return context_handle->GetIsolate();
    return v8::Isolate::GetCurrent();
  }
  static v8::Isolate* GetIsolate(
      v8::Handle<v8::ObjectTemplate> template_handle) {
    return v8::Isolate::GetCurrent();
  }
  template <typename U>
  static v8::Isolate* GetIsolate(const U& any_handle) {
    return v8::Isolate::GetCurrent();
  }

  v8::Persistent<T> handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPersistent);
};

template <typename T>
class RefCountedPersistent : public ScopedPersistent<T>,
                             public base::RefCounted<RefCountedPersistent<T>> {
 public:
  RefCountedPersistent() {}

  explicit RefCountedPersistent(v8::Handle<T> handle)
    : ScopedPersistent<T>(handle) {
  }

 protected:
  friend class base::RefCounted<RefCountedPersistent<T>>;

  ~RefCountedPersistent() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCountedPersistent);
};

}  // namespace mate

#endif  // NATIVE_MATE_SCOPED_PERSISTENT_H_
