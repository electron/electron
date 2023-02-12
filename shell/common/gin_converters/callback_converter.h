// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_

#include <utility>

#include "base/callback_helpers.h"
#include "shell/common/gin_helper/callback.h"

namespace gin {

template <typename Sig>
struct Converter<base::RepeatingCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::RepeatingCallback<Sig>& val) {
    // We don't use CreateFunctionTemplate here because it creates a new
    // FunctionTemplate everytime, which is cached by V8 and causes leaks.
    auto translater =
        base::BindRepeating(&gin_helper::NativeFunctionInvoker<Sig>::Go, val);
    // To avoid memory leak, we ensure that the callback can only be called
    // for once.
    return gin_helper::CreateFunctionFromTranslater(isolate, translater, true);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::RepeatingCallback<Sig>* out) {
    if (!val->IsFunction())
      return false;

    *out = base::BindRepeating(&gin_helper::V8FunctionInvoker<Sig>::Go, isolate,
                               gin_helper::SafeV8Function(isolate, val));
    return true;
  }
};

template <typename Sig>
struct Converter<base::OnceCallback<Sig>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   base::OnceCallback<Sig> in) {
    return gin::ConvertToV8(isolate,
                            base::AdaptCallbackForRepeating(std::move(in)));
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::OnceCallback<Sig>* out) {
    if (!val->IsFunction())
      return false;
    *out = base::BindOnce(&gin_helper::V8FunctionInvoker<Sig>::Go, isolate,
                          gin_helper::SafeV8Function(isolate, val));
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_CALLBACK_CONVERTER_H_
