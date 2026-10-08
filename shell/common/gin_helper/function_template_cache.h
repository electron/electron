// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_CACHE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_CACHE_H_

#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-template.h"

namespace gin_helper {

v8::Local<v8::FunctionTemplate> GetCachedFunctionTemplate(v8::Isolate* isolate,
                                                          const void* key);
void SetCachedFunctionTemplate(v8::Isolate* isolate,
                               const void* key,
                               v8::Local<v8::FunctionTemplate> tmpl);

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_FUNCTION_TEMPLATE_CACHE_H_
