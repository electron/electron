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

// Convert "Command" to "Ctrl" on non-Mac
std::string NormalizeShortcutSuggestion(const std::string& suggestion) {
#if defined(OS_MACOSX)
  return suggestion;
#endif

  std::string key;
  std::vector<std::string> tokens;
  base::SplitString(suggestion, '+', &tokens);
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == "Command")
      tokens[i] = "Ctrl";
  }
  return JoinString(tokens, '+');
}

}  // namespace

bool StringToAccelerator(const std::string& description,
                         ui::Accelerator* accelerator) {
  std::string shortcut(NormalizeShortcutSuggestion(description));

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
    if (tokens[i] == "Ctrl") {
      modifiers |= ui::EF_CONTROL_DOWN;
    } else if (tokens[i] == "Command") {
      modifiers |= ui::EF_COMMAND_DOWN;
    } else if (tokens[i] == "Alt") {
      modifiers |= ui::EF_ALT_DOWN;
    } else if (tokens[i] == "Shift") {
      modifiers |= ui::EF_SHIFT_DOWN;
    } else if (tokens[i].size() == 1) {
      char token = tokens[i][0];

      if (key != ui::VKEY_UNKNOWN) {
        // Multiple key assignments.
        key = ui::VKEY_UNKNOWN;
        return false;
      }
      if (token >= 'A' && token <= 'Z') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_A + (token - 'A'));
      } else if (token >= '0' && token <= '9') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_0 + (token - '0'));
      } else if (token >= '*' && token <= '/') {
        // *+,-./
        key = static_cast<ui::KeyboardCode>(
            ui::VKEY_MULTIPLY + (token - '*'));
      } else {
        switch (token) {
          case ':':
          case ';':
            key = ui::VKEY_OEM_1;
            break;
          case '?':
          case '/':
            key = ui::VKEY_OEM_2;
            break;
          case '~':
          case '`':
            key = ui::VKEY_OEM_3;
            break;
          case '{':
          case '[':
            key = ui::VKEY_OEM_4;
            break;
          case '|':
          case '\\':
            key = ui::VKEY_OEM_5;
            break;
          case '}':
          case ']':
            key = ui::VKEY_OEM_6;
            break;
          case '\"':
          case '\'':
            key = ui::VKEY_OEM_7;
            break;
          default:
            LOG(WARNING) << "Invalid accelerator character: " << tokens[i];
            key = ui::VKEY_UNKNOWN;
            return false;
        }
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
