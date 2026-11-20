// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
#define ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_

#include "build/build_config.h"
#include "electron/mas.h"

// Also included from the C entry point, shell/app/electron_main_mac.c.
#if BUILDFLAG(IS_MAC)
#ifdef __cplusplus
extern "C" {
#endif
// The entry point of the main and helper executables. Handles run-as-node and,
// when `is_helper` is nonzero, refuses to start without a --type argument,
// then calls ElectronMain().
__attribute__((visibility("default"))) int
ElectronExecutableMain(int argc, char* argv[], int is_helper);
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
#ifdef __cplusplus
}  // extern "C"
#endif
#endif

#endif  // ELECTRON_SHELL_APP_ELECTRON_LIBRARY_MAIN_H_
