// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
#define ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_

#include "build/build_config.h"
#include "electron/mas.h"

#if BUILDFLAG(IS_MAC)
extern "C" {
__attribute__((visibility("default"))) int ElectronMain(int argc, char* argv[]);
__attribute__((visibility("default"))) int ElectronInitializeICUandStartNode(
    int argc,
    char* argv[]);
#if !IS_MAS_BUILD()
// The entry point of an app shim: a small app, in a bundle of its own, that
// loads this framework from a host Electron app and shows windows on the
// host's behalf. See docs/api/app-shim-host.md. Mac App Store builds have
// neither this nor the loader that calls it.
__attribute__((visibility("default"))) int ElectronAppShimMain(int argc,
                                                               char* argv[]);
#endif
}
#endif

#endif  // ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
