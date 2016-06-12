// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "base/logging.h"

// Include common headers for using node APIs.

#define BUILDING_NODE_EXTENSION

#undef ASSERT
#undef CHECK
#undef CHECK_EQ
#undef CHECK_NE
#undef CHECK_GE
#undef CHECK_GT
#undef CHECK_LE
#undef CHECK_LT
#undef DISALLOW_COPY_AND_ASSIGN
#undef NO_RETURN
#undef arraysize
#undef debug_string  // This is defined in OS X 10.9 SDK in AssertMacros.h.
#include "vendor/node/src/env.h"
#include "vendor/node/src/env-inl.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_buffer.h"
#include "vendor/node/src/node_internals.h"
