// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "gin/arguments.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/destroyable.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/microtasks_scope.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// This file is forked from gin/function_template.h with 2 differences:
// 1. Support for additional types of arguments.
// 2. Support for warning using destroyed objects.
//
// TODO(zcbenz): We should seek to remove this file after removing native_mate.

namespace gin_helper {

enum CreateFunctionTemplateFlags {
  HolderIsFirstArgument = 1 << 0,
};

template <typename T>
struct CallbackParamTraits {
  typedef T LocalType;
};
template <typename T>
struct CallbackParamTraits<const T&> {
  typedef T LocalType;
};
template <typename T>
struct CallbackParamTraits<const T*> {
  typedef T* LocalType;
};

// CallbackHolder and CallbackHolderBase are used to pass a
// base::RepeatingCallback from
// CreateFunctionTemplate through v8 (via v8::FunctionTemplate) to
// DispatchToCallback, where it is invoked.

// This simple base class is used so that we can share a single object template
// among every CallbackHolder instance.
class CallbackHolderBase {
 public:
  v8::Local<v8::External> GetHandle(v8::Isolate* isolate);

  // disable copy
  CallbackHolderBase(const CallbackHolderBase&) = delete;
  CallbackHolderBase& operator=(const CallbackHolderBase&) = delete;

 protected:
  explicit CallbackHolderBase(v8::Isolate* isolate);
  virtual ~CallbackHolderBase();

 private:
  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);

  v8::Global<v8::External> v8_ref_;
};

template <typename Sig>
class CallbackHolder : public CallbackHolderBase {
 public:
  CallbackHolder(v8::Isolate* isolate,
                 const base::RepeatingCallback<Sig>& callback,
                 int flags)
      : CallbackHolderBase(isolate), callback(callback), flags(flags) {}
  base::RepeatingCallback<Sig> callback;
  int flags = 0;

 private:
  virtual ~CallbackHolder() = default;
};

template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     int create_flags,
                     bool is_first,
                     T* result) {
  if (is_first && (create_flags & HolderIsFirstArgument) != 0) {
    return args->GetHolder(result);
  } else {
    return args->GetNext(result);
  }
}

// Support absl::optional as output, which would be empty and do not throw error
// when conversion to T fails.
template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     int create_flags,
                     bool is_first,
                     absl::optional<T>* result) {
  T converted;
  // Use gin::Arguments::GetNext which always advances |next| counter.
  if (args->GetNext(&converted))
    result->emplace(std::move(converted));
  return true;
}

// For advanced use cases, we allow callers to request the unparsed Arguments
// object and poke around in it directly.
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            gin::Arguments** result) {
  *result = args;
  return true;
}

// It's common for clients to just need the isolate, so we make that easy.
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            v8::Isolate** result) {
  *result = args->isolate();
  return true;
}

// Allow clients to pass a util::Error to throw errors if they
// don't need the full gin::Arguments
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            ErrorThrower* result) {
  *result = ErrorThrower(args->isolate());
  return true;
}

// Supports the gin_helper::Arguments.
inline bool GetNextArgument(gin::Arguments* args,
                            int create_flags,
                            bool is_first,
                            gin_helper::Arguments** result) {
  *result = static_cast<gin_helper::Arguments*>(args);
  return true;
}

// Classes for generating and storing an argument pack of integer indices
// (based on well-known "indices trick", see: http://goo.gl/bKKojn):
template <size_t... indices>
struct IndicesHolder {};

template <size_t requested_index, size_t... indices>
struct IndicesGenerator {
  using type = typename IndicesGenerator<requested_index - 1,
                                         requested_index - 1,
                                         indices...>::type;
};
template <size_t... indices>
struct IndicesGenerator<0, indices...> {
  using type = IndicesHolder<indices...>;
};

// Class template for extracting and storing single argument for callback
// at position |index|.
template <size_t index, typename ArgType>
struct ArgumentHolder {
  using ArgLocalType = typename CallbackParamTraits<ArgType>::LocalType;

  ArgLocalType value;
  bool ok = false;

  ArgumentHolder(gin::Arguments* args, int create_flags) {
    v8::Local<v8::Object> holder;
    if (index == 0 && (create_flags & HolderIsFirstArgument) &&
        args->GetHolder(&holder) &&
        gin_helper::Destroyable::IsDestroyed(holder)) {
      args->ThrowTypeError("Object has been destroyed");
      return;
    }
    ok = GetNextArgument(args, create_flags, index == 0, &value);
    if (!ok) {
      // Ideally we would include the expected c++ type in the error
      // message which we can access via typeid(ArgType).name()
      // however we compile with no-rtti, which disables typeid.
      args->ThrowError();
    }
  }
};

// Class template for converting arguments from JavaScript to C++ and running
// the callback with them.
template <typename IndicesType, typename... ArgTypes>
class Invoker {};

template <size_t... indices, typename... ArgTypes>
class Invoker<IndicesHolder<indices...>, ArgTypes...>
    : public ArgumentHolder<indices, ArgTypes>... {
 public:
  // Invoker<> inherits from ArgumentHolder<> for each argument.
  // C++ has always been strict about the class initialization order,
  // so it is guaranteed ArgumentHolders will be initialized (and thus, will
  // extract arguments from Arguments) in the right order.
  Invoker(gin::Arguments* args, int create_flags)
      : ArgumentHolder<indices, ArgTypes>(args, create_flags)..., args_(args) {
    // GCC thinks that create_flags is going unused, even though the
    // expansion above clearly makes use of it. Per jyasskin@, casting
    // to void is the commonly accepted way to convince the compiler
    // that you're actually using a parameter/variable.
    (void)create_flags;
  }

  bool IsOK() { return And(ArgumentHolder<indices, ArgTypes>::ok...); }

  template <typename ReturnType>
  void DispatchToCallback(
      base::RepeatingCallback<ReturnType(ArgTypes...)> callback) {
    gin_helper::MicrotasksScope microtasks_scope(args_->isolate(), true);
    args_->Return(
        callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...));
  }

  // In C++, you can declare the function foo(void), but you can't pass a void
  // expression to foo. As a result, we must specialize the case of Callbacks
  // that have the void return type.
  void DispatchToCallback(base::RepeatingCallback<void(ArgTypes...)> callback) {
    gin_helper::MicrotasksScope microtasks_scope(args_->isolate(), true);
    callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
  }

 private:
  static bool And() { return true; }
  template <typename... T>
  static bool And(bool arg1, T... args) {
    return arg1 && And(args...);
  }

  gin::Arguments* args_;
};

// DispatchToCallback converts all the JavaScript arguments to C++ types and
// invokes the base::RepeatingCallback.
template <typename Sig>
struct Dispatcher {};

template <typename ReturnType, typename... ArgTypes>
struct Dispatcher<ReturnType(ArgTypes...)> {
  static void DispatchToCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    gin::Arguments args(info);
    v8::Local<v8::External> v8_holder;
    args.GetData(&v8_holder);
    CallbackHolderBase* holder_base =
        reinterpret_cast<CallbackHolderBase*>(v8_holder->Value());

    typedef CallbackHolder<ReturnType(ArgTypes...)> HolderT;
    HolderT* holder = static_cast<HolderT*>(holder_base);

    using Indices = typename IndicesGenerator<sizeof...(ArgTypes)>::type;
    Invoker<Indices, ArgTypes...> invoker(&args, holder->flags);
    if (invoker.IsOK())
      invoker.DispatchToCallback(holder->callback);
  }
};

// CreateFunctionTemplate creates a v8::FunctionTemplate that will create
// JavaScript functions that execute a provided C++ function or
// base::RepeatingCallback.
// JavaScript arguments are automatically converted via gin::Converter, as is
// the return value of the C++ function, if any.
//
// NOTE: V8 caches FunctionTemplates for a lifetime of a web page for its own
// internal reasons, thus it is generally a good idea to cache the template
// returned by this function.  Otherwise, repeated method invocations from JS
// will create substantial memory leaks. See http://crbug.com/463487.
template <typename Sig>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig> callback,
    int callback_flags = 0) {
  typedef CallbackHolder<Sig> HolderT;
  HolderT* holder = new HolderT(isolate, callback, callback_flags);

  return v8::FunctionTemplate::New(isolate,
                                   &Dispatcher<Sig>::DispatchToCallback,
                                   gin::ConvertToV8<v8::Local<v8::External>>(
                                       isolate, holder->GetHandle(isolate)));
}

// Base template - used only for non-member function pointers. Other types
// either go to one of the below specializations, or go here and fail to compile
// because of base::Bind().
template <typename T, typename Enable = void>
struct CallbackTraits {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    return gin_helper::CreateFunctionTemplate(isolate,
                                              base::BindRepeating(callback));
  }
};

// Specialization for base::RepeatingCallback.
template <typename T>
struct CallbackTraits<base::RepeatingCallback<T>> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate,
      const base::RepeatingCallback<T>& callback) {
    return gin_helper::CreateFunctionTemplate(isolate, callback);
  }
};

// Specialization for member function pointers. We need to handle this case
// specially because the first parameter for callbacks to MFP should typically
// come from the the JavaScript "this" object the function was called on, not
// from the first normal parameter.
template <typename T>
struct CallbackTraits<
    T,
    typename std::enable_if<std::is_member_function_pointer<T>::value>::type> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    int flags = HolderIsFirstArgument;
    return gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(callback), flags);
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_
