// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_INCLUDES_H_
#define ATOM_COMMON_NODE_INCLUDES_H_

#include "base/logging.h"

// Include common headers for using node APIs.

#define BUILDING_NODE_EXTENSION

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
#undef arraysize
#undef debug_string  // This is defined in macOS 10.9 SDK in AssertMacros.h.
#include "env-inl.h"
#include "env.h"
#include "node.h"
#include "node_buffer.h"
#include "node_debug_options.h"
#include "node_internals.h"
#include "node_platform.h"

namespace node {
namespace tracing {

class TraceEventHelper {
 public:
  static v8::TracingController* GetTracingController();
  static void SetTracingController(v8::TracingController* controller);
};

}  // namespace tracing
}  // namespace node

#endif  // ATOM_COMMON_NODE_INCLUDES_H_
