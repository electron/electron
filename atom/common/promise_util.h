// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_PROMISE_UTIL_H_
#define ATOM_COMMON_PROMISE_UTIL_H_

#include <string>

#include "atom/common/api/locker.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/traits_bag.h"
#include "native_mate/converter.h"

namespace atom {

namespace util {

class Promise : public base::RefCounted<Promise> {
 public:
  enum ResolutionState {
    kResolve,
    kReject,
  };

  explicit Promise(v8::Isolate* isolate);

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

 private:
  v8::Local<v8::Promise::Resolver> GetInner() const {
    return resolver_.Get(isolate());
  }

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Promise::Resolver> resolver_;

  DISALLOW_COPY_AND_ASSIGN(Promise);
};

class PromiseTraits {
 public:
  struct ValidTraits {
    ValidTraits(Promise::ResolutionState);
  };

  template <typename... Args>
  constexpr PromiseTraits(Args... args)
      : resolution_state_(
            base::trait_helpers::GetEnum<Promise::ResolutionState>(args...)) {}

  constexpr PromiseTraits(const PromiseTraits& other) = default;
  PromiseTraits& operator=(const PromiseTraits& other) = default;

  constexpr Promise::ResolutionState resolution_state() const {
    return resolution_state_;
  }

 private:
  Promise::ResolutionState resolution_state_;
};

class AdaptCallbackForPromiseHelper final {
 public:
  explicit AdaptCallbackForPromiseHelper(scoped_refptr<Promise> promise);
  ~AdaptCallbackForPromiseHelper();

  void Run(const PromiseTraits& traits) {
    if (traits.resolution_state() == Promise::kResolve) {
      promise_->Resolve();
    } else if (traits.resolution_state() == Promise::kReject) {
      promise_->Reject();
    }
  }

 private:
  scoped_refptr<Promise> promise_;

  DISALLOW_COPY_AND_ASSIGN(AdaptCallbackForPromiseHelper);
};

base::OnceCallback<void(const PromiseTraits&)> AdaptCallbackForPromise(
    scoped_refptr<Promise> promise);

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
