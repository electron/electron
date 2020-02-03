// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_PROCESS_UTIL_H_
#define SHELL_COMMON_PROCESS_UTIL_H_

#include <string>

#include "shell/common/node_includes.h"

namespace electron {

void EmitWarning(node::Environment* env,
                 const std::string& warning_msg,
                 const std::string& warning_type);

}  // namespace electron

#endif  // SHELL_COMMON_PROCESS_UTIL_H_
