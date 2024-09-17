// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_PROMISE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_PROMISE_H_

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/task_runner.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "v8/include/v8-context.h"

namespace gin_helper {

// A wrapper around the v8::Promise.
//
// This is the non-template base class to share code between templates
// instances.
//
// This is a move-only type that should always be `std::move`d when passed to
// callbacks, and it should be destroyed on the same thread of creation.
class PromiseBase {
 public:
  explicit PromiseBase(v8::Isolate* isolate);
  PromiseBase(v8::Isolate* isolate, v8::Local<v8::Promise::Resolver> handle);
  PromiseBase();
  ~PromiseBase();

  // disable copy
  PromiseBase(const PromiseBase&) = delete;
  PromiseBase& operator=(const PromiseBase&) = delete;

  // Support moving.
  PromiseBase(PromiseBase&&);
  PromiseBase& operator=(PromiseBase&&);

  // Helper for rejecting promise with error message.
  static void RejectPromise(PromiseBase&& promise, std::string_view errmsg);

  v8::Maybe<bool> Reject();
  v8::Maybe<bool> Reject(v8::Local<v8::Value> except);
  v8::Maybe<bool> RejectWithErrorMessage(std::string_view message);

  v8::Local<v8::Context> GetContext() const;
  v8::Local<v8::Promise> GetHandle() const;

  v8::Isolate* isolate() const { return isolate_; }

 protected:
  struct SettleScope {
    explicit SettleScope(const PromiseBase& base);
    ~SettleScope();

    v8::HandleScope handle_scope_;
    v8::Local<v8::Context> context_;
    gin_helper::MicrotasksScope microtasks_scope_;
    v8::Context::Scope context_scope_;
  };

  v8::Local<v8::Promise::Resolver> GetInner() const;

  static scoped_refptr<base::TaskRunner> GetTaskRunner();

 private:
  raw_ptr<v8::Isolate> isolate_;
  v8::Global<v8::Context> context_;
  v8::Global<v8::Promise::Resolver> resolver_;
};

// Template implementation that returns values.
template <typename RT>
class Promise : public PromiseBase {
 public:
  using PromiseBase::PromiseBase;

  // Helper for resolving the promise with |result|.
  static void ResolvePromise(Promise<RT> promise, RT result) {
    if (auto task_runner = GetTaskRunner()) {
      task_runner->PostTask(
          FROM_HERE, base::BindOnce([](Promise<RT> promise,
                                       RT result) { promise.Resolve(result); },
                                    std::move(promise), std::move(result)));
    } else {
      promise.Resolve(result);
    }
  }

  // Returns an already-resolved promise.
  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate,
                                                RT result) {
    Promise<RT> resolved(isolate);
    resolved.Resolve(result);
    return resolved.GetHandle();
  }

  // Convert to another type.
  template <typename NT>
  Promise<NT> As() {
    return Promise<NT>{isolate(), GetInner()};
  }

  v8::Maybe<bool> Resolve(const RT& value) {
    SettleScope settle_scope{*this};
    return GetInner()->Resolve(settle_scope.context_,
                               gin::ConvertToV8(isolate(), value));
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
    SettleScope settle_scope{*this};
    v8::Local<v8::Value> value = gin::ConvertToV8(isolate(), std::move(cb));
    v8::Local<v8::Function> handler = value.As<v8::Function>();
    return GetHandle()->Then(settle_scope.context_, handler);
  }
};

// Template implementation that returns nothing.
template <>
class Promise<void> : public PromiseBase {
 public:
  using PromiseBase::PromiseBase;

  // Helper for resolving the empty promise.
  static void ResolvePromise(Promise<void> promise);

  // Returns an already-resolved promise.
  static v8::Local<v8::Promise> ResolvedPromise(v8::Isolate* isolate);

  v8::Maybe<bool> Resolve();
};

}  // namespace gin_helper

namespace gin {

template <typename T>
struct Converter<gin_helper::Promise<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const gin_helper::Promise<T>& val) {
    return val.GetHandle();
  }
  // TODO(MarshallOfSound): Implement FromV8 to allow promise chaining
  //                        in native land
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    Promise* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_PROMISE_H_
