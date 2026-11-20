// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// The entry point for the macOS main executable and the helper executables.
//
// This is deliberately plain C: it links only libelectron_aperitif, Electron
// Framework and libSystem, in that order, so that Apéritif's initializer runs
// before any other (see shell/app/aperitif_mac.h). Keep logic in the framework.

#include <stdlib.h>

#include "shell/app/aperitif_mac.h"
#include "shell/app/electron_library_main.h"

__attribute__((constructor)) static void CheckAperitif(void) {
  ElectronAperitifCheckInitialized();
}

__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
#if defined(HELPER_EXECUTABLE)
  const int is_helper = 1;
#else
  const int is_helper = 0;
#endif
  int rv = ElectronExecutableMain(argc, argv, is_helper);

  // exit, don't return from main, to avoid the apparent removal of main from
  // stack backtraces under tail call optimization.
  exit(rv);
}
