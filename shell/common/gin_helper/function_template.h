// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_

#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "gin/arguments.h"
#include "gin/per_isolate_data.h"
#include "gin/public/gin_embedders.h"
#include "shell/common/gin_helper/destroyable.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "v8/include/cppgc/macros.h"
#include "v8/include/v8-external.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-template.h"

// This file is forked from gin/function_template.h with 2 differences:
// 1. Support for additional types of arguments.
// 2. Support for warning using destroyed objects.
//
// TODO(zcbenz): We should seek to remove this file after removing native_mate.

namespace gin_helper {

struct InvokerOptions {
  bool holder_is_first_argument = false;
  const char* holder_type = nullptr;  // Null if unknown or not applicable.
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
// base::RepeatingCallback from CreateFunctionTemplate through v8 (via
// v8::FunctionTemplate) to DispatchToCallback, where it is invoked.

// CallbackHolder will clean up the callback in two different scenarios:
// - If the garbage collector finds that it's garbage and collects it. (But note
//   that even _if_ we become garbage, we might never get collected!)
// - If the isolate gets disposed.
//
// TODO(crbug.com/1285119): When gin::Wrappable gets migrated over to using
//   cppgc, this class should also be considered for migration.

// This simple base class is used so that we can share a single object template
// among every CallbackHolder instance.
class CallbackHolderBase {
 public:
  CallbackHolderBase(const CallbackHolderBase&) = delete;
  CallbackHolderBase& operator=(const CallbackHolderBase&) = delete;

  v8::Local<v8::External> GetHandle(v8::Isolate* isolate);

  // Frees the holders created in `isolate` when it has no gin::PerIsolateData,
  // as a Node.js worker's isolate does. gin never reports the disposal of such
  // an isolate, and V8 does not run weak callbacks when it disposes one, so
  // without this the holders leak. Call it on the isolate's thread once no
  // more JavaScript will run there.
  static void DisposeAllInIsolateWithoutGin(v8::Isolate* isolate);

 protected:
  explicit CallbackHolderBase(v8::Isolate* isolate);
  virtual ~CallbackHolderBase();

 private:
  class DisposeObserver : gin::PerIsolateData::DisposeObserver {
   public:
    DisposeObserver(gin::PerIsolateData* per_isolate_data,
                    CallbackHolderBase* holder);
    ~DisposeObserver() override;

    // gin::PerIsolateData::DisposeObserver
    void OnBeforeDispose(v8::Isolate* isolate) override;
    void OnDisposed() override;

   private:
    // Unlike in Chromium, it's possible for PerIsolateData to be null
    // for a given isolate - e.g. in a Node.js Worker. Thus this
    // needs to be a raw_ptr instead of a raw_ref.
    const raw_ptr<gin::PerIsolateData> per_isolate_data_;
    const raw_ref<CallbackHolderBase> holder_;
  };

  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<CallbackHolderBase>& data);

  v8::Global<v8::External> v8_ref_;
  DisposeObserver dispose_observer_;

  // Set while this holder is registered for DisposeAllInIsolateWithoutGin().
  raw_ptr<v8::Isolate> isolate_without_gin_ = nullptr;
};

template <typename Sig>
class CallbackHolder : public CallbackHolderBase {
 public:
  CallbackHolder(v8::Isolate* isolate,
                 base::RepeatingCallback<Sig> callback,
                 InvokerOptions invoker_options)
      : CallbackHolderBase(isolate),
        callback(std::move(callback)),
        invoker_options(std::move(invoker_options)) {}
  CallbackHolder(const CallbackHolder&) = delete;
  CallbackHolder& operator=(const CallbackHolder&) = delete;

  base::RepeatingCallback<Sig> callback;
  InvokerOptions invoker_options;

 private:
  ~CallbackHolder() override = default;
};

template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     const InvokerOptions& invoker_options,
                     bool is_first,
                     T* result) {
  if (is_first && invoker_options.holder_is_first_argument) {
    return args->GetHolder(result);
  } else {
    return args->GetNext(result);
  }
}

// Electron-specific GetNextArgument that supports std::optional.
template <typename T>
bool GetNextArgument(gin::Arguments* args,
                     const InvokerOptions& invoker_options,
                     bool is_first,
                     std::optional<T>* result) {
  T converted;
  // Use gin::Arguments::GetNext which always advances |next| counter.
  if (args->GetNext(&converted))
    result->emplace(std::move(converted));
  return true;
}

// Electron-specific GetNextArgument that supports ErrorThrower.
inline bool GetNextArgument(gin::Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            ErrorThrower* result) {
  *result = ErrorThrower(args->isolate());
  return true;
}

// For advanced use cases, we allow callers to request the unparsed Arguments
// object and poke around in it directly.
inline bool GetNextArgument(gin::Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            gin::Arguments* result) {
  *result = *args;
  return true;
}

inline bool GetNextArgument(gin::Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            gin::Arguments** result) {
  *result = args;
  return true;
}

// It's common for clients to just need the isolate, so we make that easy.
inline bool GetNextArgument(gin::Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            v8::Isolate** result) {
  *result = args->isolate();
  return true;
}

// Throws an error indicating conversion failure.
void ThrowConversionError(gin::Arguments* args,
                          const InvokerOptions& invoker_options,
                          size_t index);

// Class template for extracting and storing single argument for callback
// at position |index|.
template <size_t index, typename ArgType, typename = void>
struct ArgumentHolder {
  CPPGC_STACK_ALLOCATED();

 public:
  using ArgLocalType = typename CallbackParamTraits<ArgType>::LocalType;

  ArgLocalType value;
  bool ok = false;

  ArgumentHolder(gin::Arguments* args, const InvokerOptions& invoker_options) {
    ok = GetNextArgument(args, invoker_options, index == 0, &value);
    if (!ok) {
      ThrowConversionError(args, invoker_options, index);
    }
  }
};

// This is required for types such as v8::LocalVector<T>, which don't have
// a default constructor. To create an element of such a type, the isolate
// has to be provided.
template <size_t index, typename ArgType>
struct ArgumentHolder<
    index,
    ArgType,
    std::enable_if_t<!std::is_default_constructible_v<
                         typename CallbackParamTraits<ArgType>::LocalType> &&
                     std::is_constructible_v<
                         typename CallbackParamTraits<ArgType>::LocalType,
                         v8::Isolate*>>> {
  CPPGC_STACK_ALLOCATED();

 public:
  using ArgLocalType = typename CallbackParamTraits<ArgType>::LocalType;

  ArgLocalType value;
  bool ok;

  ArgumentHolder(gin::Arguments* args, const InvokerOptions& invoker_options)
      : value(args->isolate()),
        ok(GetNextArgument(args, invoker_options, index == 0, &value)) {
    if (!ok) {
      ThrowConversionError(args, invoker_options, index);
    }
  }
};

// Blink runs microtasks when the outermost MicrotasksScope closes (kScoped),
// so a bound call in the renderer needs one. Where Node owns the checkpoint
// (kExplicit: the browser and utility processes) a scope never runs anything
// and every native-to-JS entry point already holds one, so skip it there,
// along with the creation-context lookup it needs.
class MaybeMicrotasksScope {
  CPPGC_STACK_ALLOCATED();

 public:
  explicit MaybeMicrotasksScope(gin::Arguments* args);
  ~MaybeMicrotasksScope();

 private:
  std::optional<v8::MicrotasksScope> scope_;
};

// Class template for converting arguments from JavaScript to C++ and running
// the callback with them.
template <typename IndicesType, typename... ArgTypes>
class Invoker;

template <size_t... indices, typename... ArgTypes>
class Invoker<std::index_sequence<indices...>, ArgTypes...>
    : public ArgumentHolder<indices, ArgTypes>... {
  CPPGC_STACK_ALLOCATED();

 public:
  // Invoker<> inherits from ArgumentHolder<> for each argument.
  // C++ has always been strict about the class initialization order,
  // so it is guaranteed ArgumentHolders will be initialized (and thus, will
  // extract arguments from Arguments) in the right order.
  Invoker(gin::Arguments* args, const InvokerOptions& invoker_options)
      : ArgumentHolder<indices, ArgTypes>(args, invoker_options)...,
        args_(args) {}

  [[nodiscard]] bool IsOK() const {
    return (... && ArgumentHolder<indices, ArgTypes>::ok);
  }

  template <typename ReturnType>
  void DispatchToCallback(
      const base::RepeatingCallback<ReturnType(ArgTypes...)>& callback) {
    MaybeMicrotasksScope microtasks_scope(args_);
    args_->Return(
        callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...));
  }

  // In C++, you can declare the function foo(void), but you can't pass a void
  // expression to foo. As a result, we must specialize the case of Callbacks
  // that have the void return type.
  void DispatchToCallback(
      const base::RepeatingCallback<void(ArgTypes...)>& callback) {
    MaybeMicrotasksScope microtasks_scope(args_);
    callback.Run(std::move(ArgumentHolder<indices, ArgTypes>::value)...);
  }

  // Calls a plain function or member function pointer. For a member function
  // the first converted argument is the receiver.
  template <typename Target>
  void DispatchToTarget(Target target) {
    MaybeMicrotasksScope microtasks_scope(args_);
    using ReturnType = decltype(std::invoke(
        target, std::move(ArgumentHolder<indices, ArgTypes>::value)...));
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(target,
                  std::move(ArgumentHolder<indices, ArgTypes>::value)...);
    } else {
      args_->Return(std::invoke(
          target, std::move(ArgumentHolder<indices, ArgTypes>::value)...));
    }
  }

 private:
  gin::Arguments* args_;
};

// DispatchToCallback converts all the JavaScript arguments to C++ types and
// invokes the base::RepeatingCallback.
template <typename Sig>
struct Dispatcher {};

template <typename ReturnType, typename... ArgTypes>
struct Dispatcher<ReturnType(ArgTypes...)> {
  static void DispatchToCallbackImpl(gin::Arguments* args) {
    v8::Local<v8::External> v8_holder;
    CHECK(args->GetData(&v8_holder));
    CallbackHolderBase* holder_base = reinterpret_cast<CallbackHolderBase*>(
        v8_holder->Value(v8::kExternalPointerTypeTagDefault));

    typedef CallbackHolder<ReturnType(ArgTypes...)> HolderT;
    HolderT* holder = static_cast<HolderT*>(holder_base);

    using Indices = std::index_sequence_for<ArgTypes...>;
    Invoker<Indices, ArgTypes...> invoker(args, holder->invoker_options);
    if (invoker.IsOK())
      invoker.DispatchToCallback(holder->callback);
  }

  static void DispatchToCallback(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    gin::Arguments args(info);
    DispatchToCallbackImpl(&args);
  }
};

// True for the targets that CreateFunctionTemplate<kTarget>() can dispatch to
// directly: free functions and member functions.
template <typename T>
inline constexpr bool kIsFunctionOrMethodPointer =
    std::is_member_function_pointer_v<T> ||
    (std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>);

// DirectDispatcher is the counterpart of Dispatcher for a target that is named
// at compile time. There is no CallbackHolder to find and no callback to run:
// the V8 callback converts the arguments and calls the target.
//
// The work is split in two so that targets with the same signature share
// code: SignatureDispatcher<Target> converts the arguments and calls through
// a Target passed at run time, and DirectDispatcher<kTarget> is the V8
// callback, a thunk that hands it kTarget. Where a signature has a single
// target the compiler folds the two together.
template <typename Target,
          bool is_method,
          typename IndicesType,
          typename... RunArgs>
struct SignatureDispatcherImpl;

template <typename Target,
          bool is_method,
          size_t... indices,
          typename... RunArgs>
struct SignatureDispatcherImpl<Target,
                               is_method,
                               std::index_sequence<indices...>,
                               RunArgs...> {
  static void Run(gin::Arguments* args, Target target) {
    static constexpr InvokerOptions kOptions = {.holder_is_first_argument =
                                                    is_method};
    Invoker<std::index_sequence<indices...>, RunArgs...> invoker(args,
                                                                 kOptions);
    if (invoker.IsOK()) {
      invoker.DispatchToTarget(target);
    }
  }

  static void Run(const v8::FunctionCallbackInfo<v8::Value>& info,
                  Target target) {
    gin::Arguments args(info);
    Run(&args, target);
  }
};

template <typename Target>
struct SignatureDispatcher;

template <typename R, typename... Args>
struct SignatureDispatcher<R (*)(Args...)>
    : SignatureDispatcherImpl<R (*)(Args...),
                              false,
                              std::index_sequence_for<Args...>,
                              Args...> {};

template <typename R, typename... Args>
struct SignatureDispatcher<R (*)(Args...) noexcept>
    : SignatureDispatcher<R (*)(Args...)> {};

// For member functions the receiver is converted from the JavaScript `this`,
// as ObjectTemplateBuilder has always done for member function pointers.
template <typename C, typename R, typename... Args>
struct SignatureDispatcher<R (C::*)(Args...)>
    : SignatureDispatcherImpl<R (C::*)(Args...),
                              true,
                              std::index_sequence_for<C*, Args...>,
                              C*,
                              Args...> {};

template <typename C, typename R, typename... Args>
struct SignatureDispatcher<R (C::*)(Args...) const>
    : SignatureDispatcherImpl<R (C::*)(Args...) const,
                              true,
                              std::index_sequence_for<const C*, Args...>,
                              const C*,
                              Args...> {};

template <typename C, typename R, typename... Args>
struct SignatureDispatcher<R (C::*)(Args...) noexcept>
    : SignatureDispatcher<R (C::*)(Args...)> {};

template <typename C, typename R, typename... Args>
struct SignatureDispatcher<R (C::*)(Args...) const noexcept>
    : SignatureDispatcher<R (C::*)(Args...) const> {};

template <auto kTarget>
struct DirectDispatcher {
  static void DispatchToTarget(
      const v8::FunctionCallbackInfo<v8::Value>& info) {
    SignatureDispatcher<decltype(kTarget)>::Run(info, kTarget);
  }
};

// CreateFunctionTemplate for a function or member function that is known at
// compile time:
//
//   gin_helper::CreateFunctionTemplate<&MyClass::Method>(isolate);
//
// No CallbackHolder is allocated and each call goes from V8 to the target
// without unwrapping a holder or running a callback, so prefer this form
// whenever the target is not a bound base::RepeatingCallback. A member
// function always takes its receiver from the JavaScript `this` here, whereas
// the callback form below only does so if |invoker_options| says so.
template <auto kTarget>
  requires(kIsFunctionOrMethodPointer<decltype(kTarget)>)
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(v8::Isolate* isolate) {
  return v8::FunctionTemplate::New(
      isolate, &DirectDispatcher<kTarget>::DispatchToTarget,
      v8::Local<v8::Value>(), v8::Local<v8::Signature>(), 0,
      v8::ConstructorBehavior::kAllow);
}

// CreateFunctionTemplate creates a v8::FunctionTemplate that will create
// JavaScript functions that execute a provided C++ function or
// base::RepeatingCallback. JavaScript arguments are automatically converted via
// gin::Converter, as is the return value of the C++ function, if any.
// |invoker_options| contains additional parameters. If it contains a
// holder_type, it will be used to provide a useful conversion error if the
// holder is the first argument. If not provided, a generic invocation error
// will be used.
//
// NOTE: V8 caches FunctionTemplates for a lifetime of a web page for its own
// internal reasons, thus it is generally a good idea to cache the template
// returned by this function.  Otherwise, repeated method invocations from JS
// will create substantial memory leaks. See http://crbug.com/463487.
//
// The callback will be destroyed if either the function template gets garbage
// collected or _after_ the isolate is disposed. Garbage collection can never be
// relied upon. As such, any destructors for objects bound to the callback must
// not depend on the isolate being alive at the point they are called. The order
// in which callbacks are destroyed is not guaranteed.
template <typename Sig>
v8::Local<v8::FunctionTemplate> CreateFunctionTemplate(
    v8::Isolate* isolate,
    base::RepeatingCallback<Sig> callback,
    InvokerOptions invoker_options = {}) {
  typedef CallbackHolder<Sig> HolderT;
  HolderT* holder =
      new HolderT(isolate, std::move(callback), std::move(invoker_options));

  v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(
      isolate, &Dispatcher<Sig>::DispatchToCallback,
      gin::ConvertToV8<v8::Local<v8::External>>(isolate,
                                                holder->GetHandle(isolate)),
      v8::Local<v8::Signature>(), 0, v8::ConstructorBehavior::kAllow);
  return tmpl;
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
// come from the JavaScript "this" object the function was called on, not
// from the first normal parameter.
template <typename T>
struct CallbackTraits<
    T,
    typename std::enable_if<std::is_member_function_pointer<T>::value>::type> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    InvokerOptions invoker_options = {.holder_is_first_argument = true};
    return gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(callback), std::move(invoker_options));
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_H_
