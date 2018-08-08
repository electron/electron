// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PROMISE_UTIL_H_
#define ATOM_COMMON_PROMISE_UTIL_H_

#include <string>

#include "content/public/browser/browser_thread.h"
#include "native_mate/converter.h"

namespace atom {

namespace util {

class Promise {
 public:
  explicit Promise(v8::Isolate* isolate);
  ~Promise();

  v8::Isolate* isolate() const { return isolate_; }

  virtual v8::Local<v8::Object> GetHandle() const;

  v8::Maybe<bool> Resolve() {
    return GetInner()->Resolve(isolate()->GetCurrentContext(),
                               v8::Undefined(isolate()));
  }

  v8::Maybe<bool> Reject() {
    return GetInner()->Reject(isolate()->GetCurrentContext(),
                              v8::Undefined(isolate()));
  }

  template <typename T>
  v8::Maybe<bool> Resolve(T* value) {
    return GetInner()->Resolve(isolate()->GetCurrentContext(),
                               mate::ConvertToV8(isolate(), value));
  }

  template <typename T>
  v8::Maybe<bool> Reject(T* value) {
    return GetInner()->Reject(isolate()->GetCurrentContext(),
                              mate::ConvertToV8(isolate(), value));
  }

  v8::Maybe<bool> RejectWithErrorMessage(const std::string& error);

 protected:
  v8::Isolate* isolate_;

 private:
  v8::Local<v8::Promise::Resolver> GetInner() const {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return resolver_.Get(isolate());
  }

  v8::Global<v8::Promise::Resolver> resolver_;

  DISALLOW_COPY_AND_ASSIGN(Promise);
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
