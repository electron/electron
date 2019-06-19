// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_KEYBOARD_UTIL_H_
#define SHELL_COMMON_KEYBOARD_UTIL_H_

#include <string>

#include "base/strings/string16.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace electron {

// Return key code of the char, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromCharCode(base::char16 c, bool* shifted);

// Return key code of the |str|, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted);

// Ported from ui/events/blink/blink_event_util.h
int WebEventModifiersToEventFlags(int modifiers);

}  // namespace electron

#endif  // SHELL_COMMON_KEYBOARD_UTIL_H_
