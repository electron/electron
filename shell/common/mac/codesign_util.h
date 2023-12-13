// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_MAC_CODESIGN_UTIL_H_
#define SHELL_COMMON_MAC_CODESIGN_UTIL_H_

#include <unistd.h>

namespace electron {

// Given a pid, check if the process belongs to current app by comparing its
// code signature with current app.
bool ProcessBelongToCurrentApp(pid_t pid);

}  // namespace electron

#endif  // SHELL_COMMON_MAC_CODESIGN_UTIL_H_
