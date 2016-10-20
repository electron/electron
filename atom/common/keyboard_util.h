// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_KEYBOARD_UTIL_H_
#define ATOM_COMMON_KEYBOARD_UTIL_H_

#include <string>

#include "ui/events/keycodes/keyboard_codes.h"

namespace atom {

// Return key code of the |str|, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted);

// Ported from ui/events/blink/blink_event_util.h
int WebEventModifiersToEventFlags(int modifiers);

}  // namespace atom

#endif  // ATOM_COMMON_KEYBOARD_UTIL_H_
