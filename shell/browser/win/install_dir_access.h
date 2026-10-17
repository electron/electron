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
// so every sandboxed child dies opening icudtl.dat. Checks for that before any
// child is launched and aborts with a message that names the directory and the
// missing ACL entry instead of a GPU process crash loop.
void CheckSandboxedProcessesCanReadInstallDir();

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_WIN_INSTALL_DIR_ACCESS_H_
