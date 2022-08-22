// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <cstdlib>
#include <memory>

#include "base/allocator/early_zone_registration_mac.h"
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

[[maybe_unused]] bool IsEnvSet(const char* name) {
  char* indicator = getenv(name);
  return indicator && indicator[0] != '\0';
}

}  // namespace

int main(int argc, char* argv[]) {
  bool use_custom_v8_snapshot_file_name =
      electron::fuses::IsLoadV8SnapshotFromCustomPathEnabled();
  int new_argc = argc + (use_custom_v8_snapshot_file_name ? 1 : 0);
  char* new_argv[new_argc];
  for (int i = 0; i < argc; i++) {
    new_argv[i] = argv[i];
  }
  if (use_custom_v8_snapshot_file_name) {
    new_argv[new_argc - 1] = const_cast<char*>(
        "--custom-v8-snapshot-file-name=custom_v8_context_snapshot.bin");
  }

  partition_alloc::EarlyMallocZoneRegistration();
  FixStdioStreams();
  fprintf(stderr, "main 1\n");

#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (electron::fuses::IsRunAsNodeEnabled() &&
      IsEnvSet("ELECTRON_RUN_AS_NODE")) {
    return ElectronInitializeICUandStartNode(new_argc, new_argv);
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
      sandbox::SeatbeltExecServer::CreateFromArguments(exec_path.get(),
                                                       new_argc, new_argv);
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

  return ElectronMain(new_argc, new_argv);
}
