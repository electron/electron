// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/accelerator_util.h"

#include <string>

#include "base/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "ui/base/accelerators/accelerator.h"

namespace accelerator_util {

namespace {

// For Mac, we convert "Ctrl" to "Command" and "MacCtrl" to "Ctrl". Other
// platforms leave the shortcut untouched.
std::string NormalizeShortcutSuggestion(const std::string& suggestion) {
#if !defined(OS_MACOSX)
  return suggestion;
#endif

  std::string key;
  std::vector<std::string> tokens;
  base::SplitString(suggestion, '+', &tokens);
  for (size_t i = 0; i < tokens.size(); i++) {
    if (tokens[i] == "Ctrl")
      tokens[i] = "Command";
    else if (tokens[i] == "MacCtrl")
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
  if (tokens.size() < 2 || tokens.size() > 3) {
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
      if (key != ui::VKEY_UNKNOWN) {
        // Multiple key assignments.
        key = ui::VKEY_UNKNOWN;
        break;
      }
      if (tokens[i][0] >= 'A' && tokens[i][0] <= 'Z') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_A + (tokens[i][0] - 'A'));
      } else if (tokens[i][0] >= '0' && tokens[i][0] <= '9') {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_0 + (tokens[i][0] - '0'));
      } else {
        LOG(WARNING) << "Invalid accelerator character: " << tokens[i];
        key = ui::VKEY_UNKNOWN;
        break;
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
