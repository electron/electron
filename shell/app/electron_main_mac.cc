// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <memory>

#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/app/electron_library_main.h"
#include "shell/common/electron_constants.h"

#if defined(HELPER_EXECUTABLE) && !defined(MAS_BUILD)
#include "sandbox/mac/seatbelt_exec.h"  // nogncheck
#endif

// Copied from //base/ignore_result.h, to avoid taking a dependency on //base
// on Mac.
template <typename T>
inline void ignore_result(const T&) {}

namespace {

ALLOW_UNUSED_TYPE bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

void FixStdioStreams() {
  // libuv may mark stdin/stdout/stderr as close-on-exec, which interferes
  // with chromium's subprocess spawning. As a workaround, we detect if these
  // streams are closed on startup, and reopen them as /dev/null if necessary.
  // Otherwise, an unrelated file descriptor will be assigned as stdout/stderr
  // which may cause various errors when attempting to write to them.
  //
  // For details see https://github.com/libuv/libuv/issues/2062
  struct stat st;
  if (fstat(STDIN_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "r", stdin));
  if (fstat(STDOUT_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stdout));
  if (fstat(STDERR_FILENO, &st) < 0 && errno == EBADF)
    ignore_result(freopen("/dev/null", "w", stderr));
}

}  // namespace

int main(int argc, char* argv[]) {
  FixStdioStreams();

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
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
