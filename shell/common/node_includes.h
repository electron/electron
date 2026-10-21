// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_NODE_INCLUDES_H_
#define ELECTRON_SHELL_COMMON_NODE_INCLUDES_H_

// Include common headers for using node APIs.

#ifdef NODE_SHARED_MODE
#define BUILDING_NODE_EXTENSION
#endif

#undef debug_string    // This is defined in macOS SDK in AssertMacros.h.
#undef require_string  // This is defined in macOS SDK in AssertMacros.h.

#include "electron/push_and_undef_node_defines.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"

#include "env-inl.h"
#include "env.h"
#include "node.h"
#include "node_buffer.h"
#include "node_builtins.h"
#include "node_errors.h"
#include "node_internals.h"
#include "node_options.h"
#include "tracing/agent.h"

#pragma clang diagnostic pop

#include "electron/pop_node_defines.h"

// Undefine these defines from libuv which conflict with other headers
#undef RB_BLACK
#undef RB_RED

// NODE_LINKED_BINDING_CONTEXT_AWARE
#include "shell/common/node_linked_binding.h"

#endif  // ELECTRON_SHELL_COMMON_NODE_INCLUDES_H_
