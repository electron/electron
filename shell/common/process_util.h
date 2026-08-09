// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_
#define ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_

#include <string>

namespace electron {

std::string GetProcessType();

bool IsBrowserProcess();
bool IsRendererProcess();
bool IsUtilityProcess();
bool IsZygoteProcess();

// True when this process is the ELECTRON_RUN_AS_NODE entry point (e.g. a
// child_process.fork()ed Node process). Such a process has no --type switch,
// so IsBrowserProcess() is also true for it; use this to tell the two apart.
bool IsRunningAsNode();

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_PROCESS_UTIL_H_
