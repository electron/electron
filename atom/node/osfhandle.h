// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_NODE_OSFHANDLE_H_
#define ATOM_NODE_OSFHANDLE_H_

#include <windows.h>

namespace node {

// The _open_osfhandle and _close functions on Windows are provided by the
// Visual C++ library, so the fd returned by them can only be used in the
// same instance of VC++ library.
// However Electron is linking with VC++ library statically, so electron.exe
// shares a different instance of VC++ library with node.exe. This results
// in fd created in electron.exe not usable in node.dll, so we have to ensure
// we always create fd in one instance of VC++ library.
// Followings wrappers are compiled in node.dll, and all code in electron.exe
// should call these wrappers instead of calling _open_osfhandle directly.
__declspec(dllexport) int open_osfhandle(intptr_t osfhandle, int flags);
__declspec(dllexport) int close(int fd);

}  // namespace node

#endif  // ATOM_NODE_OSFHANDLE_H_
