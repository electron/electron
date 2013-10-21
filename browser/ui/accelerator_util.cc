// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/accelerator_util.h"

#include <string>

#include "base/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "ui/base/accelerators/accelerator.h"

namespace accelerator_util {

namespace {

// Convert "Command" to "Ctrl" on non-Mac.
std::string NormalizeShortcutSuggestion(const std::string& suggestion) {
#if defined(OS_MACOSX)
  return suggestion;
#endif

  std::string key;
  std::vector<std::string> tokens;
  base::SplitString(suggestion, '+', &tokens);
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == "command")
      tokens[i] = "ctrl";
  }
  return JoinString(tokens, '+');
}

// Return key code of the char.
ui::KeyboardCode KeyboardCodeFromCharCode(char c, bool* shifted) {
  *shifted = false;
  switch (c) {
    case 8: case 0x7F: return ui::VKEY_BACK;
    case 9: return ui::VKEY_TAB;
    case 0xD: case 3: return ui::VKEY_RETURN;
    case 0x1B: return ui::VKEY_ESCAPE;
    case ' ': return ui::VKEY_SPACE;

    case 'a': return ui::VKEY_A;
    case 'b': return ui::VKEY_B;
    case 'c': return ui::VKEY_C;
    case 'd': return ui::VKEY_D;
    case 'e': return ui::VKEY_E;
    case 'f': return ui::VKEY_F;
    case 'g': return ui::VKEY_G;
    case 'h': return ui::VKEY_H;
    case 'i': return ui::VKEY_I;
    case 'j': return ui::VKEY_J;
    case 'k': return ui::VKEY_K;
    case 'l': return ui::VKEY_L;
    case 'm': return ui::VKEY_M;
    case 'n': return ui::VKEY_N;
    case 'o': return ui::VKEY_O;
    case 'p': return ui::VKEY_P;
    case 'q': return ui::VKEY_Q;
    case 'r': return ui::VKEY_R;
    case 's': return ui::VKEY_S;
    case 't': return ui::VKEY_T;
    case 'u': return ui::VKEY_U;
    case 'v': return ui::VKEY_V;
    case 'w': return ui::VKEY_W;
    case 'x': return ui::VKEY_X;
    case 'y': return ui::VKEY_Y;
    case 'z': return ui::VKEY_Z;

    case ')': *shifted = true; case '0': return ui::VKEY_0;
    case '!': *shifted = true; case '1': return ui::VKEY_1;
    case '@': *shifted = true; case '2': return ui::VKEY_2;
    case '#': *shifted = true; case '3': return ui::VKEY_3;
    case '$': *shifted = true; case '4': return ui::VKEY_4;
    case '%': *shifted = true; case '5': return ui::VKEY_5;
    case '^': *shifted = true; case '6': return ui::VKEY_6;
    case '&': *shifted = true; case '7': return ui::VKEY_7;
    case '*': *shifted = true; case '8': return ui::VKEY_8;
    case '(': *shifted = true; case '9': return ui::VKEY_9;

    case ':': *shifted = true; case ';': return ui::VKEY_OEM_1;
    case '+': *shifted = true; case '=': return ui::VKEY_OEM_PLUS;
    case '<': *shifted = true; case ',': return ui::VKEY_OEM_COMMA;
    case '_': *shifted = true; case '-': return ui::VKEY_OEM_MINUS;
    case '>': *shifted = true; case '.': return ui::VKEY_OEM_PERIOD;
    case '?': *shifted = true; case '/': return ui::VKEY_OEM_2;
    case '~': *shifted = true; case '`': return ui::VKEY_OEM_3;
    case '{': *shifted = true; case '[': return ui::VKEY_OEM_4;
    case '|': *shifted = true; case '\\': return ui::VKEY_OEM_5;
    case '}': *shifted = true; case ']': return ui::VKEY_OEM_6;
    case '"': *shifted = true; case '\'': return ui::VKEY_OEM_7;

    default: return ui::VKEY_UNKNOWN;
  }
}

}  // namespace

bool StringToAccelerator(const std::string& description,
                         ui::Accelerator* accelerator) {
  if (!IsStringASCII(description)) {
    LOG(ERROR) << "The accelerator string can only contain ASCII characters";
    return false;
  }
  std::string shortcut(StringToLowerASCII(description));
  shortcut = NormalizeShortcutSuggestion(shortcut);

  std::vector<std::string> tokens;
  base::SplitString(shortcut, '+', &tokens);
  if (tokens.size() < 2 || tokens.size() > 4) {
    LOG(WARNING) << "Invalid accelerator description: " << description;
    return false;
  }

  // Now, parse it into an accelerator.
  int modifiers = ui::EF_NONE;
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == "ctrl") {
      modifiers |= ui::EF_CONTROL_DOWN;
    } else if (tokens[i] == "command") {
      modifiers |= ui::EF_COMMAND_DOWN;
    } else if (tokens[i] == "alt") {
      modifiers |= ui::EF_ALT_DOWN;
    } else if (tokens[i] == "shift") {
      modifiers |= ui::EF_SHIFT_DOWN;
    } else if (tokens[i].size() == 1) {
      if (key != ui::VKEY_UNKNOWN) {
        // Multiple key assignments.
        key = ui::VKEY_UNKNOWN;
        return false;
      }

      bool shifted = false;
      key = KeyboardCodeFromCharCode(tokens[i][0], &shifted);
      if (key == ui::VKEY_UNKNOWN) {
        LOG(WARNING) << "Invalid accelerator token: " << tokens[i];
        return false;
      } else if (shifted) {
        modifiers |= ui::EF_SHIFT_DOWN;
      }
    } else {
      LOG(WARNING) << "Invalid accelerator token: " << tokens[i];
      return false;
    }
  }

  *accelerator = ui::Accelerator(key, modifiers);
  SetPlatformAccelerator(accelerator);
  return true;
}

}  // namespace accelerator_util
