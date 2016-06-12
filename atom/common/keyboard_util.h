// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "ui/events/keycodes/keyboard_codes.h"

namespace atom {

// Return key code of the |str|, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted);

}  // namespace atom
