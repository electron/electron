// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_PROMISE_UTIL_H_
#define SHELL_COMMON_PROMISE_UTIL_H_

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/strings/string_piece.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/converter.h"
#include "native_mate/microtasks_scope.h"
#include "shell/common/gin_converters/std_converter.h"

namespace electron {

namespace util {

// A wrapper around the v8::Promise.
//
// This is a move-only type that should always be `std::move`d when passed to
// callbacks, and it should be destroyed on the same thread of creation.
template <typename RT>
class Promise {
 public:
  // Create a new promise.
  explicit Promise(v8::Isolate* isolate)
      : Promise(isolate,
                v8::Promise::Resolver::New(isolate->GetCurrentContext())
                    .ToLocalChecked()) {}

  // Wrap an existing v8 promise.
  Promise(v8::Isolate* isolate, v8::Local<v8::Promise::Resolver> handle)
      : isolate_(isolate),
        context_(isolate, isolate->GetCurrentContext()),
        resolver_(isolate, handle) {}

  ~Promise() = default;

  // Support moving.
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Context> GetContext() {
    return v8::Local<v8::Context>::New(isolate_, context_);
  }

  // helpers for promise resolution and rejection

  static void ResolvePromise(Promise<RT> promise, RT result) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      base::PostTask(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce([](Promise<RT> promise,
                            RT result) { promise.ResolveWithGin(result); },
                         std::move(promise), std::move(result)));
    } else {
      promise.ResolveWithGin(result);
    }
  }

  static void ResolveEmptyPromise(Promise<RT> promise) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      base::PostTask(
          FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce([](Promise<RT> promise) { promise.Resolve(); },
                         std::move(promise)));
    } else {
      promise.Resolve();
    }
  }

  static void RejectPromise(Promise<RT> promise, base::StringPiece errmsg) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
      base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                     base::BindOnce(
                         [](Promise<RT> promise, base::StringPiece err) {
                           promise.RejectWithErrorMessage(err);
                         },
                         std::move(promise), std::move(errmsg)));
    } else {
      promise.RejectWithErrorMessage(errmsg);
    }
  }

  // Returns an already-resolved promise.
  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate,
                                                RT result) {
    Promise<RT> resolved(isolate);
    resolved.Resolve(result);
    return resolved.GetHandle();
  }

  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate) {
    Promise<void*> resolved(isolate);
    resolved.Resolve();
    return resolved.GetHandle();
  }

  v8::Local<v8::Promise> GetHandle() const { return GetInner()->GetPromise(); }

  v8::Maybe<bool> Resolve() {
    static_assert(std::is_same<void*, RT>(),
                  "Can only resolve void* promises with no value");
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Resolve(GetContext(), v8::Undefined(isolate()));
  }

  v8::Maybe<bool> Reject() {
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Reject(GetContext(), v8::Undefined(isolate()));
  }

  v8::Maybe<bool> Reject(v8::Local<v8::Value> exception) {
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Reject(GetContext(), exception);
  }

  template <typename... ResolveType>
  v8::MaybeLocal<v8::Promise> Then(
      base::OnceCallback<void(ResolveType...)> cb) {
    static_assert(sizeof...(ResolveType) <= 1,
                  "A promise's 'Then' callback should only receive at most one "
                  "parameter");
    static_assert(
        std::is_same<RT, std::tuple_element_t<0, std::tuple<ResolveType...>>>(),
        "A promises's 'Then' callback must handle the same type as the "
        "promises resolve type");
    v8::HandleScope handle_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    v8::Local<v8::Value> value = gin::ConvertToV8(isolate(), std::move(cb));
    v8::Local<v8::Function> handler = v8::Local<v8::Function>::Cast(value);

    return GetHandle()->Then(GetContext(), handler);
  }

  // Promise resolution is a microtask
  // We use the MicrotasksRunner to trigger the running of pending microtasks
  v8::Maybe<bool> Resolve(const RT& value) {
    static_assert(!std::is_same<void*, RT>(),
                  "void* promises can not be resolved with a value");
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Resolve(GetContext(),
                               mate::ConvertToV8(isolate(), value));
  }

  v8::Maybe<bool> ResolveWithGin(const RT& value) {
    static_assert(!std::is_same<void*, RT>(),
                  "void* promises can not be resolved with a value");
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    return GetInner()->Resolve(GetContext(),
                               gin::ConvertToV8(isolate(), value));
  }

  v8::Maybe<bool> RejectWithErrorMessage(base::StringPiece string) {
    v8::HandleScope handle_scope(isolate());
    mate::MicrotasksScope microtasks_scope(isolate());
    v8::Context::Scope context_scope(
        v8::Local<v8::Context>::New(isolate(), GetContext()));

    v8::Local<v8::Value> error =
        v8::Exception::Error(gin::StringToV8(isolate(), string));
    return GetInner()->Reject(GetContext(), (error));
  }

 private:
  v8::Local<v8::Promise::Resolver> GetInner() const {
    return resolver_.Get(isolate());
  }

  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Promise::Resolver> resolver_;

  DISALLOW_COPY_AND_ASSIGN(Promise);
};

}  // namespace util

}  // namespace electron

namespace mate {

template <typename T>
struct Converter<electron::util::Promise<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const electron::util::Promise<T>& val);
  // TODO(MarshallOfSound): Implement FromV8 to allow promise chaining
  //                        in native land
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    Promise* out);
};

}  // namespace mate

#endif  // SHELL_COMMON_PROMISE_UTIL_H_
