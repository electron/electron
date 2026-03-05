// Copyright (c) 2026 Anysphere, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_OOM_STACK_TRACE_H_
#define ELECTRON_SHELL_RENDERER_OOM_STACK_TRACE_H_

#include "v8/include/v8-isolate.h"

namespace electron {

void RegisterOomStackTraceCallback(v8::Isolate* isolate);

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_OOM_STACK_TRACE_H_
