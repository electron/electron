// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PROMISE_UTIL_H_
#define ATOM_COMMON_PROMISE_UTIL_H_

#include <string>

#include "atom/common/api/locker.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/converter.h"

namespace atom {

namespace util {

class Promise : public base::RefCounted<Promise> {
 public:
  explicit Promise(v8::Isolate* isolate);

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> GetContext() {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  virtual v8::Local<v8::Promise> GetHandle() const;

  v8::Maybe<bool> Resolve() {
    v8::HandleScope handle_scope(isolate());
    v8::MicrotasksScope script_scope(isolate(),
                                     v8::MicrotasksScope::kRunMicrotasks);
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Resolve(GetContext(), v8::Undefined(isolate()));
  }

  v8::Maybe<bool> Reject() {
    v8::HandleScope handle_scope(isolate());
    v8::MicrotasksScope script_scope(isolate(),
                                     v8::MicrotasksScope::kRunMicrotasks);
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Reject(GetContext(), v8::Undefined(isolate()));
  }

  // Promise resolution is a microtask
  // We use the MicrotasksRunner to trigger the running of pending microtasks
  template <typename T>
  v8::Maybe<bool> Resolve(const T& value) {
    v8::HandleScope handle_scope(isolate());
    v8::MicrotasksScope script_scope(isolate(),
                                     v8::MicrotasksScope::kRunMicrotasks);
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Resolve(GetContext(),
                               mate::ConvertToV8(isolate(), value));
  }

  template <typename T>
  v8::Maybe<bool> Reject(const T& value) {
    v8::HandleScope handle_scope(isolate());
    v8::MicrotasksScope script_scope(isolate(),
                                     v8::MicrotasksScope::kRunMicrotasks);
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Reject(GetContext(),
                              mate::ConvertToV8(isolate(), value));
  }

  v8::Maybe<bool> RejectWithErrorMessage(const std::string& error);

 protected:
  virtual ~Promise();
  friend class base::RefCounted<Promise>;
  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;

 private:
  v8::Local<v8::Promise::Resolver> GetInner() const {
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
