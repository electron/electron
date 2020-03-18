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

namespace context_bridge {
class RenderFrameFunctionStore;

enum ProxyMode {
  // Traditionally non-transferable objects will be smartly proxied, this
  // includes functions and promises
  kSmart,
  // Follows classic serialization / de-serialization strategy
  kSimple
};

}  // namespace context_bridge

v8::Local<v8::Value> ProxyFunctionWrapper(
    context_bridge::RenderFrameFunctionStore* store,
    size_t func_id,
    context_bridge::ProxyMode proxy_mode,
    gin_helper::Arguments* args);

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& target_context,
    context_bridge::RenderFrameFunctionStore* store,
    context_bridge::ObjectCache* object_cache,
    context_bridge::ProxyMode proxy_mode,
    int recursion_depth);

}  // namespace api

}  // namespace electron

#endif  // SHELL_RENDERER_API_ELECTRON_API_CONTEXT_BRIDGE_H_
