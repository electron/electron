// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "base/strings/cstring_view.h"
#include "electron/fuses.h"
#include "electron/mas.h"
#include "shell/app/electron_library_main.h"
#include "shell/app/uv_stdio_fix.h"
#include "shell/common/electron_constants.h"

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
#include <mach-o/dyld.h>
#include <cstdio>

#include "sandbox/mac/seatbelt_exec.h"  // nogncheck
#endif

extern "C" {
// abort_report_np() records the message in a special section that both the
// system CrashReporter and Crashpad collect in crash reports. Using a Crashpad
// Annotation would be preferable, but this executable cannot depend on
// Crashpad directly.
void abort_report_np(const char* fmt, ...);
}

namespace {

[[nodiscard]] bool IsEnvSet(const base::cstring_view name) {
  const char* const indicator = getenv(name.c_str());
  return indicator && *indicator;
}

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
[[noreturn]] void FatalError(const std::string errmsg) {
  if (!errmsg.empty()) {
    std::cerr << errmsg;
    abort_report_np("%s", errmsg.c_str());
  }
  abort();
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    return ElectronInitializeICUandStartNode(argc, argv);
  }

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
  uint32_t exec_path_size = 0;
  int rv = _NSGetExecutablePath(nullptr, &exec_path_size);
  if (rv != -1) {
    FatalError("_NSGetExecutablePath: get length failed.");
  }

  auto exec_path = std::make_unique<char[]>(exec_path_size);
  rv = _NSGetExecutablePath(exec_path.get(), &exec_path_size);
  if (rv != 0) {
    FatalError("_NSGetExecutablePath: get path failed.");
  }
  sandbox::SeatbeltExecServer::CreateFromArgumentsResult seatbelt =
      sandbox::SeatbeltExecServer::CreateFromArguments(exec_path.get(), argc,
                                                       argv);
  if (seatbelt.sandbox_required) {
    if (!seatbelt.server) {
      FatalError("Failed to create seatbelt sandbox server.");
    }
    if (!seatbelt.server->InitializeSandbox()) {
      FatalError("Failed to initialize sandbox.");
    }
  }
#endif  // defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD

  return ElectronMain(argc, argv);
}
