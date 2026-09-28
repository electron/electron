// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_ELECTRON_API_COMMAND_LINE_H_
#define ELECTRON_SHELL_COMMON_API_ELECTRON_API_COMMAND_LINE_H_

namespace gin_helper {
class Dictionary;
}

namespace electron::api {

// Adds hasSwitch, getSwitchValue, appendSwitch, appendArgument and
// removeSwitch for the current process's command line to |dict|.
void FillCommandLine(gin_helper::Dictionary* dict);

}  // namespace electron::api

#endif  // ELECTRON_SHELL_COMMON_API_ELECTRON_API_COMMAND_LINE_H_
