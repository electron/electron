// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WIN_INSTALL_DIR_ACCESS_H_
#define ELECTRON_SHELL_BROWSER_WIN_INSTALL_DIR_ACCESS_H_

namespace electron {

// Sandboxed child processes run with a restricted token. When the install
// directory's ACL carries an ACE for any AppContainer package SID but none for
// ALL APPLICATION PACKAGES, Windows evaluates that token's restricting SIDs
// like an AppContainer and denies the children read access to the directory,
// so every sandboxed child dies opening icudtl.dat. Per-user install
// locations inherit such ACEs from other software; Program Files already
// grants ALL APPLICATION PACKAGES. Probes the directory with the sandbox's
// token before any child is launched and, if the probe is denied, adds an
// inheritable read/execute ACE for ALL APPLICATION PACKAGES to it.
void EnsureSandboxedProcessesCanReadInstallDir();

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WIN_INSTALL_DIR_ACCESS_H_
