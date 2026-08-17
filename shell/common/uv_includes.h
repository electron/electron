// Copyright (c) 2026 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_UV_INCLUDES_H_
#define ELECTRON_SHELL_COMMON_UV_INCLUDES_H_

#include <uv.h>

// Undefine these defines from libuv which conflict with other headers
#undef RB_BLACK
#undef RB_RED

#endif  // ELECTRON_SHELL_COMMON_UV_INCLUDES_H_
