// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PROMISE_UTIL_H_
#define ATOM_COMMON_PROMISE_UTIL_H_

#include <string>

#include "native_mate/converter.h"

namespace atom {

namespace util {

class Promise {
 public:
  explicit Promise(v8::Isolate* isolate) {
    isolate_ = isolate;
    resolver_.Reset(isolate, v8::Promise::Resolver::New(isolate));
  }
  ~Promise() { resolver_.Reset(); }

  v8::Isolate* isolate() const { return isolate_; }

  virtual v8::Local<v8::Object> GetHandle() const;

  Promise* Resolve() {
    GetInner()->Resolve(v8::Undefined(isolate()));
    return this;
  }

  Promise* Reject() {
    GetInner()->Reject(v8::Undefined(isolate()));
    return this;
  }

  template <typename T>
  Promise* Resolve(T* value) {
    GetInner()->Resolve(mate::ConvertToV8(isolate(), value));
    return this;
  }

  template <typename T>
  Promise* Reject(T* value) {
    GetInner()->Reject(mate::ConvertToV8(isolate(), value));
    return this;
  }

  Promise* RejectWithErrorMessage(const std::string& error);

 protected:
  v8::Isolate* isolate_;

 private:
  v8::Local<v8::Promise::Resolver> GetInner() const {
    return resolver_.Get(isolate());
  }

  v8::Global<v8::Promise::Resolver> resolver_;

  // DISALLOW_COPY_AND_ASSIGN(Promise);
};

}  // namespace util

}  // namespace atom

namespace mate {

template <>
struct Converter<atom::util::Promise*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   atom::util::Promise* val);
  // TODO(MarshallOfSound): Implement FromV8 to allow promise chaining
  //                        in native land
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    Promise* out);
};

}  // namespace mate

#endif  // ATOM_COMMON_PROMISE_UTIL_H_
