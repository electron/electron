// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NODE_INCLUDES_H_
#define ATOM_COMMON_NODE_INCLUDES_H_

// Include common headers for using node APIs.

#undef CHECK
#undef CHECK_EQ
#undef CHECK_NE
#undef DISALLOW_COPY_AND_ASSIGN
#include "vendor/node/src/env.h"
#include "vendor/node/src/env-inl.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_buffer.h"
#include "vendor/node/src/node_internals.h"
using node::node_isolate;

namespace atom {
// Defined in node_bindings.cc.
// For renderer it's created in atom_renderer_client.cc.
// For browser it's created in atom_browser_main_parts.cc.
extern node::Environment* global_env;
}

#endif  // ATOM_COMMON_NODE_INCLUDES_H_
