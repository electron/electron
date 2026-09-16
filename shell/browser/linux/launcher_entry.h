// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LINUX_LAUNCHER_ENTRY_H_
#define ELECTRON_SHELL_BROWSER_LINUX_LAUNCHER_ENTRY_H_

#include <cstdint>

// Emits com.canonical.Unity.LauncherEntry Update signals on the session bus.
// Despite the name, this D-Bus API is the de facto protocol for taskbar badge
// counts and progress bars on Linux.
namespace electron::launcher_entry {

// Sets the badge count on the app's launcher entry. A count of zero hides
// the badge. Returns false when the app's desktop name is not known.
// Otherwise the signal is emitted best-effort and whether anything renders
// it depends on the desktop environment.
bool SetBadgeCount(int64_t count);

// Sets the progress bar on the app's launcher entry. Values outside (0, 1)
// hide the progress bar. Returns false under the same conditions as
// SetBadgeCount().
bool SetProgress(double progress);

}  // namespace electron::launcher_entry

#endif  // ELECTRON_SHELL_BROWSER_LINUX_LAUNCHER_ENTRY_H_
