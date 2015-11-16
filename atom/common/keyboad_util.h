// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_KEYBOAD_UTIL_H_
#define ATOM_COMMON_KEYBOAD_UTIL_H_

#include <string>
#include "ui/events/keycodes/keyboard_codes.h"
#include "base/strings/string_util.h"

namespace atom {

// Return key code of the char, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromCharCode(base::char16 c, bool* shifted);

// Return key code of the char from a string representation of the char
ui::KeyboardCode KeyboardCodeFromKeyIdentifier(const std::string& chr);

}  // namespace atom

#endif  // ATOM_COMMON_KEYBOAD_UTIL_H_
