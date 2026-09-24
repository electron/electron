// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/node_entry_scope.h"

namespace gin_helper {

NodeEntryScope::NodeEntryScope(v8::Local<v8::Context> context,
                               v8::Local<v8::Object> resource) {
  node::Environment* env = node::Environment::GetCurrent(context);
  if (!env || env->async_callback_scope_depth() > 0)
    return;
  // async_context{0, 0} with hooks skipped: the resource is not observable,
  // same as gin_helper::EmitEvent.
  scope_.emplace(env, resource, node::async_context{0, 0},
                 node::InternalCallbackScope::kSkipAsyncHooks);
}

NodeEntryScope::~NodeEntryScope() = default;

}  // namespace gin_helper
