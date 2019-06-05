// Copyright (c) 2019 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_ONCE_CALLBACK_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_ONCE_CALLBACK_H_

#include <utility>

#include "atom/common/native_mate_converters/callback.h"

namespace mate {

namespace internal {

// Manages the OnceCallback with ref-couting.
template <typename Sig>
class RefCountedOnceCallback
    : public base::RefCounted<RefCountedOnceCallback<Sig>> {
 public:
  explicit RefCountedOnceCallback(base::OnceCallback<Sig> callback)
      : callback_(std::move(callback)) {}

  base::OnceCallback<Sig> GetCallback() { return std::move(callback_); }

 private:
  friend class base::RefCounted<RefCountedOnceCallback<Sig>>;
  ~RefCountedOnceCallback() = default;

  base::OnceCallback<Sig> callback_;
};

// Invokes the OnceCallback.
template <typename Sig>
struct InvokeOnceCallback {};

template <typename... ArgTypes>
struct InvokeOnceCallback<void(ArgTypes...)> {
  static void Go(
      scoped_refptr<RefCountedOnceCallback<void(ArgTypes...)>> holder,
      ArgTypes... args) {
    base::OnceCallback<void(ArgTypes...)> callback = holder->GetCallback();
    DCHECK(!callback.is_null());
    std::move(callback).Run(std::move(args)...);
  }
};

template <typename ReturnType, typename... ArgTypes>
struct InvokeOnceCallback<ReturnType(ArgTypes...)> {
  static ReturnType Go(
      scoped_refptr<RefCountedOnceCallback<ReturnType(ArgTypes...)>> holder,
      ArgTypes... args) {
    base::OnceCallback<void(ArgTypes...)> callback = holder->GetCallback();
    DCHECK(!callback.is_null());
    return std::move(callback).Run(std::move(args)...);
  }
};

}  // namespace internal

template <typename Sig>
struct Converter<base::OnceCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   base::OnceCallback<Sig> val) {
    // Reuse the converter of base::RepeatingCallback by storing the callback
    // with a RefCounted.
    auto holder = base::MakeRefCounted<internal::RefCountedOnceCallback<Sig>>(
        std::move(val));
    return Converter<base::RepeatingCallback<Sig>>::ToV8(
        isolate,
        base::BindRepeating(&internal::InvokeOnceCallback<Sig>::Go, holder));
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::OnceCallback<Sig>* out) {
    if (!val->IsFunction())
      return false;
    *out = base::BindOnce(&internal::V8FunctionInvoker<Sig>::Go, isolate,
                          internal::SafeV8Function(isolate, val));
    return true;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_ONCE_CALLBACK_H_
