// Copyright (c) 2026 Electron contributors.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_APP_ELECTRON_RUNTIME_WIN_H_
#define ELECTRON_SHELL_APP_ELECTRON_RUNTIME_WIN_H_

#include <windows.h>

namespace sandbox {
struct SandboxInterfaceInfo;
}

namespace electron {

using InitializeSandboxInfoCallback = void (*)(sandbox::SandboxInterfaceInfo*);
using RuntimeMain = int (*)(HINSTANCE,
                            InitializeSandboxInfoCallback,
                            const volatile char*);

}  // namespace electron

#endif  // ELECTRON_SHELL_APP_ELECTRON_RUNTIME_WIN_H_
