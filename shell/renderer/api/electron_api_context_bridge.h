// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
#define ELECTRON_SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_

#include "shell/renderer/api/context_bridge/object_cache.h"
#include "v8/include/v8.h"

namespace gin_helper {
class Arguments;
}

namespace electron::api {

void ProxyFunctionWrapper(const v8::FunctionCallbackInfo<v8::Value>& info);

// Where the context bridge should create the exception it is about to throw
enum class BridgeErrorTarget {
  // The source / calling context.  This is default and correct 99% of the time,
  // the caller / context asking for the conversion will receive the error and
  // therefore the error should be made in that context
  kSource,
  // The destination / target context.  This should only be used when the source
  // won't catch the error that results from the value it is passing over the
  // bridge.  This can **only** occur when returning a value from a function as
  // we convert the return value after the method has terminated and execution
  // has been returned to the caller.  In this scenario the error will the be
  // catchable in the "destination" context and therefore we create the error
  // there.
  kDestination
};

v8::MaybeLocal<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth,
    BridgeErrorTarget error_target = BridgeErrorTarget::kSource);

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth);

}  // namespace electron::api

#endif  // ELECTRON_SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
