// Copyright (c) 2015 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_CALLBACK_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_CALLBACK_H_

#include <vector>

#include "atom/common/api/locker.h"
#include "base/bind.h"
#include "base/callback.h"
#include "native_mate/function_template.h"
#include "native_mate/scoped_persistent.h"
#include "third_party/WebKit/public/web/WebScopedMicrotaskSuppression.h"

namespace mate {

namespace internal {

typedef scoped_refptr<RefCountedPersistent<v8::Function> > SafeV8Function;

// Helper to invoke a V8 function with C++ parameters.
template <typename Sig>
struct V8FunctionInvoker {};

template <typename... ArgTypes>
struct V8FunctionInvoker<v8::Local<v8::Value>(ArgTypes...)> {
  static v8::Local<v8::Value> Go(v8::Isolate* isolate, SafeV8Function function,
                                 ArgTypes... raw) {
    Locker locker(isolate);
    v8::EscapableHandleScope handle_scope(isolate);
    scoped_ptr<blink::WebScopedRunV8Script> script_scope(
        Locker::IsBrowserProcess() ?
        nullptr : new blink::WebScopedRunV8Script(isolate));
    v8::Local<v8::Function> holder = function->NewHandle();
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    std::vector<v8::Local<v8::Value>> args = { ConvertToV8(isolate, raw)... };
    v8::Local<v8::Value> ret(holder->Call(holder, args.size(), &args.front()));
    return handle_scope.Escape(ret);
  }
};

template <typename... ArgTypes>
struct V8FunctionInvoker<void(ArgTypes...)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function,
                 ArgTypes... raw) {
    Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    scoped_ptr<blink::WebScopedRunV8Script> script_scope(
        Locker::IsBrowserProcess() ?
        nullptr : new blink::WebScopedRunV8Script(isolate));
    v8::Local<v8::Function> holder = function->NewHandle();
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    std::vector<v8::Local<v8::Value>> args = { ConvertToV8(isolate, raw)... };
    holder->Call(holder, args.size(), &args.front());
  }
};

template <typename ReturnType, typename... ArgTypes>
struct V8FunctionInvoker<ReturnType(ArgTypes...)> {
  static ReturnType Go(v8::Isolate* isolate, SafeV8Function function,
                       ArgTypes... raw) {
    Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    scoped_ptr<blink::WebScopedRunV8Script> script_scope(
        Locker::IsBrowserProcess() ?
        nullptr : new blink::WebScopedRunV8Script(isolate));
    v8::Local<v8::Function> holder = function->NewHandle();
    v8::Local<v8::Context> context = holder->CreationContext();
    v8::Context::Scope context_scope(context);
    ReturnType ret = ReturnType();
    std::vector<v8::Local<v8::Value>> args = { ConvertToV8(isolate, raw)... };
    v8::Local<v8::Value> result;
    auto maybe_result =
        holder->Call(context, holder, args.size(), &args.front());
    if (maybe_result.ToLocal(&result))
      Converter<ReturnType>::FromV8(isolate, result, &ret);
    return ret;
  }
};

// Helper to pass a C++ funtion to JavaScript.
using Translater = base::Callback<void(Arguments* args)>;
v8::Local<v8::Value> CreateFunctionFromTranslater(
    v8::Isolate* isolate, const Translater& translater);

// Calls callback with Arguments.
template <typename Sig>
struct NativeFunctionInvoker {};

template <typename ReturnType, typename... ArgTypes>
struct NativeFunctionInvoker<ReturnType(ArgTypes...)> {
  static void Go(base::Callback<ReturnType(ArgTypes...)> val, Arguments* args) {
    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(args, 0);
    if (invoker.IsOK())
      invoker.DispatchToCallback(val);
  }
};

// Create a static function that accepts generic callback.
template <typename Sig>
Translater ConvertToTranslater(const base::Callback<Sig>& val) {
  return base::Bind(&NativeFunctionInvoker<Sig>::Go, val);
}

}  // namespace internal

template<typename Sig>
struct Converter<base::Callback<Sig> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Callback<Sig>& val) {
    // We don't use CreateFunctionTemplate here because it creates a new
    // FunctionTemplate everytime, which is cached by V8 and causes leaks.
    internal::Translater translater = internal::ConvertToTranslater(val);
    return internal::CreateFunctionFromTranslater(isolate, translater);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Callback<Sig>* out) {
    if (!val->IsFunction())
      return false;

    internal::SafeV8Function function(
        new RefCountedPersistent<v8::Function>(isolate, val));
    *out = base::Bind(&internal::V8FunctionInvoker<Sig>::Go, isolate, function);
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_CALLBACK_H_
