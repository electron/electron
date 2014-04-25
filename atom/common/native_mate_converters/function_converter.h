// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_FUNCTION_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_FUNCTION_CONVERTER_H_

#include "atom/common/browser_v8_locker.h"
#include "base/bind.h"
#include "base/callback.h"
#include "native_mate/constructor.h"
#include "native_mate/scoped_persistent.h"

namespace mate {

namespace internal {

typedef scoped_refptr<RefCountedPersistent<v8::Function> > SafeV8Function;

// Helper to convert type to V8 with storage type (const T& to T).
template<typename T>
v8::Handle<v8::Value> ConvertToV8(v8::Isolate* isolate, T a) {
  return Converter<typename base::internal::CallbackParamTraits<T>::StorageType>
      ::ToV8(isolate, a);
}

// This set of templates invokes a V8::Function by converting the C++ types.
template<typename Sig>
struct V8FunctionInvoker;

template<typename R>
struct V8FunctionInvoker<R()> {
  static R Go(v8::Isolate* isolate, SafeV8Function function) {
    R ret;
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> val(holder->Call(holder, 0, NULL));
    Converter<R>::FromV8(isolate, val, &ret);
    return ret;
  }
};

template<>
struct V8FunctionInvoker<void()> {
  static void Go(v8::Isolate* isolate, SafeV8Function function) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    holder->Call(holder, 0, NULL);
  }
};

template<typename R, typename P1>
struct V8FunctionInvoker<R(P1)> {
  static R Go(v8::Isolate* isolate, SafeV8Function function, P1 a1) {
    R ret;
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
    };
    v8::Handle<v8::Value> val(holder->Call(holder, arraysize(args), args));
    Converter<R>::FromV8(isolate, val, &ret);
    return ret;
  }
};

template<typename P1>
struct V8FunctionInvoker<void(P1)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function, P1 a1) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
    };
    holder->Call(holder, arraysize(args), args);
  }
};

template<typename R, typename P1, typename P2>
struct V8FunctionInvoker<R(P1, P2)> {
  static R Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2) {
    R ret;
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
    };
    v8::Handle<v8::Value> val(holder->Call(holder, arraysize(args), args));
    Converter<R>::FromV8(isolate, val, &ret);
    return ret;
  }
};

template<typename P1, typename P2>
struct V8FunctionInvoker<void(P1, P2)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
    };
    holder->Call(holder, arraysize(args), args);
  }
};

template<typename R, typename P1, typename P2, typename P3>
struct V8FunctionInvoker<R(P1, P2, P3)> {
  static R Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2,
      P3 a3) {
    R ret;
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
        ConvertToV8(isolate, a3),
    };
    v8::Handle<v8::Value> val(holder->Call(holder, arraysize(args), args));
    Converter<R>::FromV8(isolate, val, &ret);
    return ret;
  }
};

template<typename P1, typename P2, typename P3>
struct V8FunctionInvoker<void(P1, P2, P3)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2,
      P3 a3) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
        ConvertToV8(isolate, a3),
    };
    holder->Call(holder, arraysize(args), args);
  }
};

template<typename R, typename P1, typename P2, typename P3, typename P4>
struct V8FunctionInvoker<R(P1, P2, P3, P4)> {
  static R Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2,
      P3 a3, P4 a4) {
    R ret;
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
        ConvertToV8(isolate, a3),
        ConvertToV8(isolate, a4),
    };
    v8::Handle<v8::Value> val(holder->Call(holder, arraysize(args), args));
    Converter<R>::FromV8(isolate, val, &ret);
    return ret;
  }
};

template<typename P1, typename P2, typename P3, typename P4>
struct V8FunctionInvoker<void(P1, P2, P3, P4)> {
  static void Go(v8::Isolate* isolate, SafeV8Function function, P1 a1, P2 a2,
      P3 a3, P4 a4) {
    atom::BrowserV8Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Handle<v8::Function> holder = function->NewHandle();
    v8::Handle<v8::Value> args[] = {
        ConvertToV8(isolate, a1),
        ConvertToV8(isolate, a2),
        ConvertToV8(isolate, a3),
        ConvertToV8(isolate, a4),
    };
    holder->Call(holder, arraysize(args), args);
  }
};

}  // namespace internal

template<typename Sig>
struct Converter<base::Callback<Sig> > {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const base::Callback<Sig>& val) {
    return CreateFunctionTemplate(isolate, val)->GetFunction();
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::Callback<Sig>* out) {
    if (!val->IsFunction())
      return false;

    internal::SafeV8Function function(
        new RefCountedPersistent<v8::Function>(val));
    *out = base::Bind(&internal::V8FunctionInvoker<Sig>::Go, isolate, function);
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_FUNCTION_CONVERTER_H_
