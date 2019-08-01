// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_GIN_UTILS_H_
#define SHELL_COMMON_API_GIN_UTILS_H_

#include "gin/function_template.h"

namespace gin {

// NOTE: V8 caches FunctionTemplates. Therefore it is user's responsibility
// to ensure this function is called for one type only ONCE in the program's
// whole lifetime, otherwise we would have memory leak.
template <typename Sig>
v8::Local<v8::Function> ConvertCallbackToV8Leaked(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig>& callback) {
  // LOG(WARNING) << "Leaky conversion from callback to V8 triggered.";
  return gin::CreateFunctionTemplate(isolate, callback)
      ->GetFunction(isolate->GetCurrentContext())
      .ToLocalChecked();
}

}  // namespace gin

#endif  // SHELL_COMMON_API_GIN_UTILS_H_
