// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_ENTRY_SCOPE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_ENTRY_SCOPE_H_

#include <optional>

#include "base/memory/stack_allocated.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8-forward.h"

namespace gin_helper {

// Put one of these on the stack before calling from native code into a JS
// function. If nothing above it has entered JS through Node (callback scope
// depth 0), it opens a node::InternalCallbackScope so that process.nextTick
// callbacks and the microtask queue drain when the call returns, in that
// order, as they do for every callback Node itself makes. If JS is already on
// the stack it does nothing: the outermost scope owns the drain, and Node
// skips it at depth > 1 anyway.
class NodeEntryScope {
  STACK_ALLOCATED();

 public:
  NodeEntryScope(v8::Local<v8::Context> context,
                 v8::Local<v8::Object> resource);
  ~NodeEntryScope();

  NodeEntryScope(const NodeEntryScope&) = delete;
  NodeEntryScope& operator=(const NodeEntryScope&) = delete;

 private:
  std::optional<node::InternalCallbackScope> scope_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_NODE_ENTRY_SCOPE_H_
