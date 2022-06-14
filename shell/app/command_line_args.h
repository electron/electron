// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_COMMAND_LINE_ARGS_H_
#define ELECTRON_SHELL_APP_COMMAND_LINE_ARGS_H_

#include "base/command_line.h"

namespace electron {

bool CheckCommandLineArguments(int argc, base::CommandLine::CharType** argv);
bool IsSandboxEnabled(base::CommandLine* command_line);

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_COMMAND_LINE_ARGS_H_
