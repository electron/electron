// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_LINUX_GLOBAL_SHORTCUTS_PORTAL_LISTER_H_
#define ELECTRON_SHELL_BROWSER_LINUX_GLOBAL_SHORTCUTS_PORTAL_LISTER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"

namespace electron {

// A shortcut registered with the org.freedesktop.portal.GlobalShortcuts
// desktop portal.
struct PortalGlobalShortcut {
  std::string id;
  std::string description;
  std::string trigger_description;
};

using ListPortalGlobalShortcutsCallback = base::OnceCallback<void(
    std::optional<std::vector<PortalGlobalShortcut>> shortcuts)>;

// Asynchronously lists the shortcuts registered with the GlobalShortcuts
// desktop portal for this application, including shortcuts approved during
// previous application sessions, using a long-lived dedicated portal
// session. Runs `callback` with std::nullopt on failure. Must be called on
// the UI thread.
void ListPortalGlobalShortcuts(ListPortalGlobalShortcutsCallback callback);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_LINUX_GLOBAL_SHORTCUTS_PORTAL_LISTER_H_
