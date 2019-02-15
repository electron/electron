// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_INCLUDES_H_
#define ATOM_COMMON_NODE_INCLUDES_H_

#include "base/logging.h"

// Include common headers for using node APIs.

#ifdef NODE_SHARED_MODE
#define BUILDING_NODE_EXTENSION
#endif

// The following define makes sure that we do not include the macros
// again. But we still need the tracing functions, so declaring them.
#define SRC_TRACING_TRACE_EVENT_H_

#undef ASSERT
#undef CHECK
#undef CHECK_EQ
#undef CHECK_NE
#undef CHECK_GE
#undef CHECK_GT
#undef CHECK_LE
#undef CHECK_LT
#undef UNLIKELY
#undef DISALLOW_COPY_AND_ASSIGN
#undef NO_RETURN
#undef LIKELY
#undef debug_string    // This is defined in macOS SDK in AssertMacros.h.
#undef require_string  // This is defined in macOS SDK in AssertMacros.h.
#include "env-inl.h"
#include "env.h"
#include "node.h"
#include "node_buffer.h"
#include "node_internals.h"
#include "node_options.h"
#include "node_platform.h"

namespace node {
namespace tracing {

class TraceEventHelper {
 public:
  static v8::TracingController* GetTracingController();
  static node::tracing::Agent* GetAgent();
  static void SetAgent(node::tracing::Agent* agent);
};

}  // namespace tracing
}  // namespace node

#endif  // ATOM_COMMON_NODE_INCLUDES_H_
