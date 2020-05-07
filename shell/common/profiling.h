// Copyright (c) 2019 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SRC_ELECTRON_SHELL_COMMON_PROFILING_H_
#define SRC_ELECTRON_SHELL_COMMON_PROFILING_H_

#include "base/strings/string_piece.h"
#include "v8/include/v8.h"

namespace electron {

namespace profiling {

void Mark(base::StringPiece key);

}  // namespace profiling

v8::Local<v8::Value> GetProfilingMarks(v8::Isolate* isolate);

}  // namespace electron

#endif  // SRC_ELECTRON_SHELL_COMMON_PROFILING_H_
