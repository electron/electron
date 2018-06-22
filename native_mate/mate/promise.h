// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef NATIVE_MATE_PROMISE_H_
#define NATIVE_MATE_PROMISE_H_

#include "native_mate/converter.h"

namespace mate {

class Promise {
 public:
  Promise();
  Promise(v8::Isolate* isolate);
  virtual ~Promise();

  static Promise Create(v8::Isolate* isolate);
  static Promise Create();

  v8::Isolate* isolate() const { return isolate_; }

  virtual v8::Local<v8::Object> GetHandle() const;

  template<typename T>
  void Resolve(T* value) {
    resolver_->Resolve(mate::ConvertToV8(isolate(), value));
  }

  template<typename T>
  void Reject(T* value) {
    resolver_->Reject(mate::ConvertToV8(isolate(), value));
  }

  void RejectWithErrorMessage(const std::string& error);

 protected:
  v8::Isolate* isolate_;

 private:
  v8::Local<v8::Promise::Resolver> resolver_;
};

template<>
struct Converter<Promise> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    Promise val);
  // TODO(MarshallOfSound): Implement FromV8 to allow promise chaining
  //                        in native land
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    Promise* out);
};

}  // namespace mate

#endif  // NATIVE_MATE_PROMISE_H_
