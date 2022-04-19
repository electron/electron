// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <memory>

#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/app/electron_library_main.h"
#include "shell/app/uv_stdio_fix.h"

#if defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)
#include <mach-o/dyld.h>
#include <cstdio>

#include "sandbox/mac/seatbelt_exec.h"  // nogncheck
#endif

namespace {

ALLOW_UNUSED_TYPE bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet("ELECTRON_RUN_AS_NODE")) {
    return ElectronInitializeICUandStartNode(argc, argv);
  }
#endif

#if defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)
  uint32_t exec_path_size = 0;
  int rv = _NSGetExecutablePath(NULL, &exec_path_size);
  if (rv != -1) {
    fprintf(stderr, "_NSGetExecutablePath: get length failed\n");
    abort();
  }

  auto exec_path = std::make_unique<char[]>(exec_path_size);
  rv = _NSGetExecutablePath(exec_path.get(), &exec_path_size);
  if (rv != 0) {
    fprintf(stderr, "_NSGetExecutablePath: get path failed\n");
    abort();
  }
  sandbox::SeatbeltExecServer::CreateFromArgumentsResult seatbelt =
      sandbox::SeatbeltExecServer::CreateFromArguments(exec_path.get(), argc,
                                                       argv);
  if (seatbelt.sandbox_required) {
    if (!seatbelt.server) {
      fprintf(stderr, "Failed to create seatbelt sandbox server.\n");
      abort();
    }
    if (!seatbelt.server->InitializeSandbox()) {
      fprintf(stderr, "Failed to initialize sandbox.\n");
      abort();
    }
  }
#endif  // defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)

  return ElectronMain(argc, argv);
}
