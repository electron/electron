// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
#define SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_

#include "shell/renderer/api/context_bridge/object_cache.h"
#include "v8/include/v8.h"

namespace gin_helper {
class Arguments;
}

namespace electron {

namespace api {

void ProxyFunctionWrapper(const v8::FunctionCallbackInfo<v8::Value>& info);

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& target_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth);

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
