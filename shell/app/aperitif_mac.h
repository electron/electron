// Copyright (c) 2026 Electron contributors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_APERITIF_MAC_H_
#define ELECTRON_SHELL_APP_APERITIF_MAC_H_

// Electron's Apéritif library, modeled on //content/public/app/aperitif_mac.h.
//
// The main and helper executables are built from electron_main_mac.c, a plain
// C file that links only this library, Electron Framework and libSystem, in
// that order. dyld therefore runs this library's static initializer before any
// initializer in Electron Framework or in the system libraries it links. The
// initializer:
//   1. registers PartitionAlloc's early malloc zone, before the framework's
//      allocator shim initializes;
//   2. repairs closed stdio file descriptors; and
//   3. in helpers launched with seatbelt arguments (not in Mac App Store
//      builds), engages the sandbox.
//
// Linking to this library has those side effects. Keep its dependencies to
// libSystem and libsandbox: anything it links runs before the sandbox is on.

#ifdef __cplusplus
extern "C" {
#endif

// Terminates the process if Apéritif has not finished initializing.
__attribute__((visibility("default"))) void ElectronAperitifCheckInitialized(
    void);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // ELECTRON_SHELL_APP_APERITIF_MAC_H_
