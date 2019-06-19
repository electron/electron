// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_APP_COMMAND_LINE_ARGS_H_
#define SHELL_APP_COMMAND_LINE_ARGS_H_

#include "base/command_line.h"

namespace electron {

bool CheckCommandLineArguments(int argc, base::CommandLine::CharType** argv);

}  // namespace electron

#endif  // SHELL_APP_COMMAND_LINE_ARGS_H_
