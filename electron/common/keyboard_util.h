// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_COMMON_KEYBOARD_UTIL_H_
#define ELECTRON_COMMON_KEYBOARD_UTIL_H_

#include <string>

#include "ui/events/keycodes/keyboard_codes.h"

namespace electron {

// Return key code of the |str|, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted);

}  // namespace electron

#endif  // ELECTRON_COMMON_KEYBOARD_UTIL_H_
