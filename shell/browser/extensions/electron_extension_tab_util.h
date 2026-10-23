// Copyright (c) 2026 Anthropic PBC
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_UTIL_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_UTIL_H_

#include <optional>
#include <string>

#include "base/types/expected.h"
#include "electron/shell/common/extensions/api/tabs.h"

class ExtensionFunction;

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

namespace electron::api {
class WebContents;
}  // namespace electron::api

namespace extensions {

class Extension;

// Whether |web_contents| is a tab to the extensions of its session. This is
// the one answer behind tab lookup by ID, the tab and window IDs stamped on
// network requests, and `sender.tab`.
//
// A WebContents registered with Extensions.addTab() is a tab only if it was
// added as one: a registered popup or side panel never is. An unregistered
// WebContents is a tab only while its session treats every WebContents as one
// (ElectronTabMode::kAllWebContents, the default).
bool IsTab(content::WebContents* web_contents);

// Resolves |tab_id| to an electron::api::WebContents only if the underlying
// WebContents is a tab (see IsTab()) and belongs to |browser_context|. Tabs in
// other BrowserContexts are treated as nonexistent so that an extension loaded
// into one Session cannot observe or operate on windows belonging to another
// Session, matching Chrome's profile-scoped ExtensionTabUtil::GetTabById
// semantics.
electron::api::WebContents* GetElectronTabById(
    int tab_id,
    content::BrowserContext* browser_context);

// The window "current window" means to the caller of |function|: the window
// of the registered view that sent it, else the window the app reported as
// focused. Unset if there is neither.
std::optional<int> GetCurrentWindowId(ExtensionFunction* function);

// Resolves the tab a chrome.tabs function targets. With |tab_id| unset this is
// the active tab of the caller's current window, and an error if there is no
// such window or tab: it never falls back to some other WebContents.
base::expected<electron::api::WebContents*, std::string> ResolveTab(
    ExtensionFunction* function,
    std::optional<int> tab_id);

// "title", "url" and "favIconUrl" are privileged: an extension sees them only
// with the "tabs" permission or access to the tab's origin.
bool CanAccessPrivilegedTabFields(const Extension* extension,
                                  electron::api::WebContents* contents);

// Builds the chrome.tabs.Tab for |contents|, which must be a tab. The fields
// Electron derives from the WebContents win over the fields the app registered
// for tabs.json properties, which win over the app's custom properties.
//
// With |extension| set, the fields it may not see are removed. Pass null only
// where the tab is the sender of a message, which the receiving extension must
// be able to identify.
electron_api::tabs::Tab CreateTabObject(electron::api::WebContents* contents,
                                        const Extension* extension);

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_EXTENSION_TAB_UTIL_H_
