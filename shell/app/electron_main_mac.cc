// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <memory>

#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/app/electron_library_main.h"
#include "shell/app/uv_stdio_fix.h"

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

bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
[[noreturn]] void FatalError(const char* format, ...) {
  va_list valist;
  va_start(valist, format);
  char message[4096];
  if (vsnprintf(message, sizeof(message), format, valist) >= 0) {
    fputs(message, stderr);
    abort_report_np("%s", message);
  }
  va_end(valist);
  abort();
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

  if (electron::fuses::IsRunAsNodeEnabled() &&
      IsEnvSet("ELECTRON_RUN_AS_NODE")) {
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
