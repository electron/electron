// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// Code here runs before the sandbox is engaged, so anything it acquires stays
// available to an unprivileged process. Keep dependencies to the minimum; see
// shell/app/aperitif_mac.h.

#include "shell/app/aperitif_mac.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/cdefs.h>

#include "base/allocator/early_zone_registration_apple.h"
#include "base/compiler_specific.h"
#include "electron/mas.h"
#include "shell/app/uv_stdio_fix.h"

#if !IS_MAS_BUILD()
#include <crt_externs.h>
#include <limits.h>
#include <mach-o/dyld.h>

#include "sandbox/mac/seatbelt_exec.h"
#endif

extern "C" {
// abort_report_np() records the message in a special section that both the
// system CrashReporter and Crashpad collect in crash reports. Using a Crashpad
// Annotation would be preferable, but this library cannot depend on Crashpad.
void abort_report_np(const char* fmt, ...) __abortlike __printflike(1, 2);
}

namespace electron::aperitif {
namespace {

[[clang::not_tail_called]] [[noreturn]] [[gnu::format(printf, 1, 2)]] void
FatalError(const char* format, ...) {
  va_list valist;
  va_start(valist, format);
  char message[4096];
  int rv = UNSAFE_TODO(vsnprintf(message, sizeof(message), format, valist));
  va_end(valist);
  if (rv >= 0) {
    UNSAFE_TODO(fprintf(stderr, "aperitif: %s\n", message));
    fflush(stderr);
    UNSAFE_TODO(abort_report_np("aperitif: %s", message));
  }
  abort();
}

// Its constructor is this library's static initializer.
class Initializer {
 public:
  Initializer() {
    // Must run once, before the process becomes multithreaded and before
    // Electron Framework's allocator shim initializes.
    partition_alloc::EarlyMallocZoneRegistration();

    // Opening /dev/null may not be allowed once the sandbox is on.
    FixStdioStreams();

#if !IS_MAS_BUILD()
    // Mac App Store builds use the App Sandbox instead of Seatbelt.
    InitializeSandbox();
#endif

    initialized_ = true;
  }

  ~Initializer() = default;

  bool initialized() const { return initialized_; }

 private:
#if !IS_MAS_BUILD()
  void InitializeSandbox() {
    char exec_path[PATH_MAX];
    uint32_t exec_path_size = sizeof(exec_path);
    if (_NSGetExecutablePath(exec_path, &exec_path_size) != 0) {
      FatalError("_NSGetExecutablePath: get path failed");
    }

    // Engages the sandbox only when the browser launched this process with
    // seatbelt arguments; the browser process and run-as-node never are.
    sandbox::SeatbeltExecServer::CreateFromArgumentsResult seatbelt =
        sandbox::SeatbeltExecServer::CreateFromArguments(
            exec_path, *_NSGetArgc(), *_NSGetArgv());
    if (seatbelt.sandbox_required) {
      if (!seatbelt.server) {
        FatalError("Failed to create seatbelt sandbox server");
      }
      if (!seatbelt.server->InitializeSandbox()) {
        FatalError("Failed to initialize sandbox");
      }
    }
  }
#endif

  bool initialized_ = false;
};

Initializer g_aperitif;

}  // namespace
}  // namespace electron::aperitif

extern "C" {

void ElectronAperitifCheckInitialized(void) {
  if (!electron::aperitif::g_aperitif.initialized()) {
    electron::aperitif::FatalError(
        "ElectronAperitifCheckInitialized: Apéritif has not been initialized");
  }
}

}  // extern "C"
