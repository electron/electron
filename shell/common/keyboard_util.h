// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_KEYBOARD_UTIL_H_
#define ELECTRON_SHELL_COMMON_KEYBOARD_UTIL_H_

<<<<<<< HEAD
#include <optional>

#include "base/strings/string_piece.h"
=======
#include <string_view>

#include "third_party/abseil-cpp/absl/types/optional.h"
>>>>>>> 47d2f8d56e (chore: migrate from base::StringPiece to std::string_view in keyboard_util.cc)
#include "ui/events/keycodes/keyboard_codes.h"

namespace electron {

// Return key code of the |str|, if the original key is a shifted character,
// for example + and /, set it in |shifted_char|.
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(std::string_view str,
                                     std::optional<char16_t>* shifted_char);

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_KEYBOARD_UTIL_H_
