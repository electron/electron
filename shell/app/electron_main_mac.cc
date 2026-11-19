// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <sysexits.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "base/compiler_specific.h"
#include "base/strings/cstring_view.h"
#include "electron/fuses.h"
#include "electron/mas.h"
#include "shell/app/electron_library_main.h"
#include "shell/app/uv_stdio_fix.h"
#include "shell/common/electron_constants.h"

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
#include "content/public/app/aperitif_mac.h"  // nogncheck
#endif

namespace {

#if defined(HELPER_EXECUTABLE) && !IS_MAS_BUILD()
// libaperitif starts the sandbox (when the browser passed seatbelt arguments)
// from its own static initializer. It is linked before Electron Framework, so
// that runs before any code from the framework or from system libraries other
// than libSystem. This constructor runs after every library's initializers and
// makes sure that libaperitif was loaded and finished initializing.
__attribute__((constructor)) void CheckAperitif() {
  AperitifCheckInitialized();
}
#endif

[[nodiscard]] bool IsEnvSet(const base::cstring_view name) {
  const char* const indicator = getenv(name.c_str());
  return indicator && *indicator;
}

#if defined(HELPER_EXECUTABLE)
// Helper apps only ever host child processes, which are always launched with
// --type. Started bare (for example by child_process.fork() while the
// runAsNode fuse is disabled) a helper would otherwise boot a second browser
// process out of the helper bundle, whose own children then fail to launch.
[[nodiscard]] bool HasProcessType(int argc, char* argv[]) {
  constexpr std::string_view kProcessType = "--type=";
  for (int i = 1; i < argc; ++i) {
    // SAFETY: the OS guarantees that argv holds argc entries.
    if (std::string_view(UNSAFE_BUFFERS(argv[i])).starts_with(kProcessType))
      return true;
  }
  return false;
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
  // In a helper, the sandbox may already be on here (see CheckAperitif()).
  // Chromium launches child processes with valid stdio, so this only reopens
  // streams for processes started some other way, such as run-as-node, which
  // are not sandboxed.
  FixStdioStreams();

  if (electron::fuses::IsRunAsNodeEnabled() && IsEnvSet(electron::kRunAsNode)) {
    return ElectronInitializeICUandStartNode(argc, argv);
  }

#if defined(HELPER_EXECUTABLE)
  if (!HasProcessType(argc, argv)) {
    std::cerr << "This is a helper executable and cannot be launched directly; "
                 "it requires a --type argument from the browser process."
              << std::endl;
    return EX_USAGE;
  }
#endif

  return ElectronMain(argc, argv);
}
