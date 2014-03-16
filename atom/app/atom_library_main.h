// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_LIBRARY_MAIN_H_
#define ATOM_APP_ATOM_LIBRARY_MAIN_H_

#include "base/basictypes.h"

#if defined(OS_MACOSX)
extern "C" {
__attribute__((visibility("default")))
int AtomMain(int argc, const char* argv[]);
}
#endif  // OS_MACOSX

#endif  // ATOM_APP_ATOM_LIBRARY_MAIN_H_
