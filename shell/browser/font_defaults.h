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

void SetFontDefaults(blink::web_pref::WebPreferences* prefs);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_FONT_DEFAULTS_H_
