// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_
#define ATOM_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_

#include <map>
#include <string>
#include <tuple>

#include "atom/common/node_includes.h"
#include "atom/renderer/api/context_bridge/render_frame_context_bridge_store.h"
#include "atom/renderer/atom_render_frame_observer.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace atom {

namespace api {

v8::Local<v8::Value> ProxyFunctionWrapper(
    context_bridge::RenderFramePersistenceStore* store,
    size_t func_id,
    mate::Arguments* args);

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& target_context,
    context_bridge::RenderFramePersistenceStore* store,
    int recursion_depth);

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_API_ATOM_API_CONTEXT_BRIDGE_H_
