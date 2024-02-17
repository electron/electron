// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_MAC_CODESIGN_UTIL_H_
#define SHELL_COMMON_MAC_CODESIGN_UTIL_H_

#include <unistd.h>

namespace electron {

// Given a pid, return true if the process has the same code signature with
// with current app.
// This API returns true if current app is not signed or ad-hoc signed, because
// checking code signature is meaningless in this case, and failing the
// signature check would break some features with unsigned binary (for example,
// process.send stops working in processes created by child_process.fork, due
// to the NODE_CHANNEL_ID env getting removed).
bool ProcessSignatureIsSameWithCurrentApp(pid_t pid);

}  // namespace electron

#endif  // SHELL_COMMON_MAC_CODESIGN_UTIL_H_
