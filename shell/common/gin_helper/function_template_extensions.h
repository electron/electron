// Copyright 2020 Slack Technologies, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
#define SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_

#include <utility>

#include "gin/function_template.h"
#include "shell/common/gin_helper/error_thrower.h"

// This extends the functionality in //gin/function_template.h for "special"
// arguments to gin-bound methods.
// It's the counterpart to function_template.h, which includes these methods
// in the gin_helper namespace.
namespace gin {

// Support base::Optional as an argument.
template <typename T>
bool GetNextArgument(Arguments* args,
                     const InvokerOptions& invoker_options,
                     bool is_first,
                     base::Optional<T>* result) {
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

}  // namespace gin

#endif  // SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_EXTENSIONS_H_
