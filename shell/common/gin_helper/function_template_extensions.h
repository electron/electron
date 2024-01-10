// Copyright 2020 Slack Technologies, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_

#include <utility>

#include "gin/function_template.h"
#include "shell/common/gin_helper/error_thrower.h"

// This extends the functionality in //gin/function_template.h for "special"
// arguments to gin-bound methods.
// It's the counterpart to function_template.h, which includes these methods
// in the gin_helper namespace.
namespace gin {

// Support std::optional as an argument.
template <typename T>
bool GetNextArgument(Arguments* args,
                     const InvokerOptions& invoker_options,
                     bool is_first,
                     std::optional<T>* result) {
  T converted;
  // Use gin::Arguments::GetNext which always advances |next| counter.
  if (args->GetNext(&converted))
    result->emplace(std::move(converted));
  return true;
}

inline bool GetNextArgument(Arguments* args,
                            const InvokerOptions& invoker_options,
                            bool is_first,
                            gin_helper::ErrorThrower* result) {
  *result = gin_helper::ErrorThrower(args->isolate());
  return true;
}

// Like gin::CreateFunctionTemplate, but doesn't remove the template's
// prototype.
template <typename Sig>
v8::Local<v8::FunctionTemplate> CreateConstructorFunctionTemplate(
    v8::Isolate* isolate,
    base::RepeatingCallback<Sig> callback,
    InvokerOptions invoker_options = {}) {
  typedef internal::CallbackHolder<Sig> HolderT;
  HolderT* holder =
      new HolderT(isolate, std::move(callback), std::move(invoker_options));

  v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(
      isolate, &internal::Dispatcher<Sig>::DispatchToCallback,
      ConvertToV8<v8::Local<v8::External>>(isolate,
                                           holder->GetHandle(isolate)));
  return tmpl;
}

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
