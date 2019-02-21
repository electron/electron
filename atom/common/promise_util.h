// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PROMISE_UTIL_H_
#define ATOM_COMMON_PROMISE_UTIL_H_

#include <string>

#include "atom/common/api/locker.h"
#include "atom/common/native_mate_converters/callback.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/converter.h"

namespace atom {

namespace util {

// A wrapper around the v8::Promise.
//
// This is a move-only type that should always be `std::move`d when passed to
// callbacks, and it should be destroyed on the same thread of creation.
class Promise {
 public:
  // Create a new promise.
  explicit Promise(v8::Isolate* isolate);

  // Wrap an existing v8 promise.
  Promise(v8::Isolate* isolate, v8::Local<v8::Promise::Resolver> handle);

  ~Promise();

  // Support moving.
  Promise(Promise&&);
  Promise& operator=(Promise&&);

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> GetContext() {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  v8::Local<v8::Promise> GetHandle() const;

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

  template <typename ReturnType, typename... ArgTypes>
  v8::MaybeLocal<v8::Promise> Then(base::Callback<ReturnType(ArgTypes...)> cb) {
    v8::HandleScope handle_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    v8::Local<v8::Value> value = mate::ConvertToV8(isolate(), cb);
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::Cast(value);

    return GetHandle()->Then(GetContext(), handler);
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

 private:
  friend class CopyablePromise;

  v8::Local<v8::Promise::Resolver> GetInner() const {
    return resolver_.Get(isolate());
  }

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Promise::Resolver> resolver_;

  DISALLOW_COPY_AND_ASSIGN(Promise);
};

// A wrapper of Promise that can be copied.
//
// This class should only be used when we have to pass Promise to a Chromium API
// that does not take OnceCallback.
class CopyablePromise {
 public:
  explicit CopyablePromise(const Promise& promise);
  CopyablePromise(const CopyablePromise&);
  ~CopyablePromise();

  Promise GetPromise() const;

 private:
  using CopyablePersistent =
      v8::CopyablePersistentTraits<v8::Promise::Resolver>::CopyablePersistent;

  v8::Isolate* isolate_;
  CopyablePersistent handle_;
};

}  // namespace util

}  // namespace atom

namespace mate {

template <>
struct Converter<atom::util::Promise> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const atom::util::Promise& val);
  // TODO(MarshallOfSound): Implement FromV8 to allow promise chaining
  //                        in native land
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    Promise* out);
};

}  // namespace mate

#endif  // ATOM_COMMON_PROMISE_UTIL_H_
