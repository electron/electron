// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
#define ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
extern "C" {
__attribute__((visibility("default"))) int ElectronMain(int argc, char* argv[]);
__attribute__((visibility("default"))) int ElectronInitializeICUandStartNode(
    int argc,
    char* argv[]);
}
#endif

#endif  // ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
