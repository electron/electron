// Copyright (c) 2018 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_FONT_DEFAULTS_H_
#define ATOM_BROWSER_FONT_DEFAULTS_H_

namespace content {
struct WebPreferences;
}  // namespace content

namespace atom {

void SetFontDefaults(content::WebPreferences* prefs);

}  // namespace atom

#endif  // ATOM_BROWSER_FONT_DEFAULTS_H_
