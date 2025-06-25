// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FONT_DEFAULTS_H_
#define ELECTRON_SHELL_BROWSER_FONT_DEFAULTS_H_

namespace blink {
namespace web_pref {
struct WebPreferences;
}  // namespace web_pref
}  // namespace blink

namespace electron {

// Set the default font preferences. The functionality is copied from
// chrome/browser/prefs_tab_helper.cc with modifications to work
// without a preference service and cache chrome/browser/font_family_cache.cc
// that persists across app sessions.
// Keep the core logic in sync to avoid performance regressions
// Refs https://issues.chromium.org/issues/400473071
void SetFontDefaults(blink::web_pref::WebPreferences* prefs);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_FONT_DEFAULTS_H_
