// Copyright 2014 Cheng Zhao. All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#ifndef NATIVE_MATE_SCOPED_PERSISTENT_H_
#define NATIVE_MATE_SCOPED_PERSISTENT_H_

#include "base/memory/ref_counted.h"
#include "native_mate/converter.h"
#include "v8/include/v8.h"

namespace mate {

// A v8::Persistent handle to a V8 value which destroys and clears the
// underlying handle on destruction.
template <typename T>
class ScopedPersistent {
 public:
  ScopedPersistent() : isolate_(v8::Isolate::GetCurrent()) {}

  ScopedPersistent(v8::Isolate* isolate, v8::Local<v8::Value> handle)
      : isolate_(isolate) {
    reset(isolate, v8::Local<T>::Cast(handle));
  }

  ~ScopedPersistent() {
    reset();
  }

  void reset(v8::Isolate* isolate, v8::Local<T> handle) {
    if (!handle.IsEmpty()) {
      isolate_ = isolate;
      handle_.Reset(isolate, handle);
    } else {
      reset();
    }
  }

  void reset() {
    handle_.Reset();
  }

  bool IsEmpty() const {
    return handle_.IsEmpty();
  }

  v8::Local<T> NewHandle() const {
    return NewHandle(isolate_);
  }

  v8::Local<T> NewHandle(v8::Isolate* isolate) const {
    if (handle_.IsEmpty())
      return v8::Local<T>();
    return v8::Local<T>::New(isolate, handle_);
  }

  template<typename P, typename C>
  void SetWeak(P* parameter, C callback) {
    handle_.SetWeak(parameter, callback);
  }

  v8::Isolate* isolate() const { return isolate_; }

 private:
  v8::Isolate* isolate_;
  v8::Persistent<T> handle_;

  DISALLOW_COPY_AND_ASSIGN(ScopedPersistent);
};

template <typename T>
class RefCountedPersistent : public ScopedPersistent<T>,
                             public base::RefCounted<RefCountedPersistent<T>> {
 public:
  RefCountedPersistent() {}

  RefCountedPersistent(v8::Isolate* isolate, v8::Local<v8::Value> handle)
    : ScopedPersistent<T>(isolate, handle) {
  }

 protected:
  friend class base::RefCounted<RefCountedPersistent<T>>;

  ~RefCountedPersistent() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCountedPersistent);
};

template<typename T>
struct Converter<ScopedPersistent<T> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const ScopedPersistent<T>& val) {
    return val.NewHandle(isolate);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ScopedPersistent<T>* out) {
    v8::Local<T> converted;
    if (!Converter<v8::Local<T> >::FromV8(isolate, val, &converted))
      return false;

    out->reset(isolate, converted);
    return true;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_SCOPED_PERSISTENT_H_
