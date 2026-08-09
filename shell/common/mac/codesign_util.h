// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_MAC_CODESIGN_UTIL_H_
#define SHELL_COMMON_MAC_CODESIGN_UTIL_H_

#include <Security/Security.h>

#include <optional>

namespace electron {

// Retrieves the audit token of the parent process via its task name port.
// Callers should invoke this as early in process startup as possible.
std::optional<audit_token_t> GetParentProcessAuditToken();

// Given a process audit token, return true if the process has the same code
// signature as the current app. An audit token is bound to a single process
// instance (it incorporates a per-exec version) so, unlike a PID, it cannot be
// reused by a process that exec()s into a different image after the token is
// captured.
// This API returns true if current app is not signed or ad-hoc signed, because
// checking code signature is meaningless in this case, and failing the
// signature check would break some features with unsigned binary (for example,
// process.send stops working in processes created by child_process.fork, due
// to the NODE_CHANNEL_ID env getting removed).
bool ProcessSignatureIsSameWithCurrentApp(audit_token_t audit_token);

}  // namespace electron

#endif  // SHELL_COMMON_MAC_CODESIGN_UTIL_H_
