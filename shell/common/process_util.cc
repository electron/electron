// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/process_util.h"

#include <string>
#include <string_view>

#include "base/command_line.h"
#include "content/public/common/content_switches.h"

namespace electron {

std::string GetProcessType() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  return command_line->GetSwitchValueASCII(switches::kProcessType);
}

bool IsBrowserProcess() {
  static bool result = GetProcessType().empty();
  return result;
}

bool IsRendererProcess() {
  static bool result = GetProcessType() == switches::kRendererProcess;
  return result;
}

bool IsUtilityProcess() {
  static bool result = GetProcessType() == switches::kUtilityProcess;
  return result;
}

bool IsZygoteProcess() {
  static bool result = GetProcessType() == switches::kZygoteProcess;
  return result;
}

}  // namespace electron
