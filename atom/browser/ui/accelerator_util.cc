// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/accelerator_util.h"

#include <stdio.h>

#include <string>
#include <vector>

#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "ui/base/models/simple_menu_model.h"

namespace accelerator_util {

namespace {

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
  if (!base::IsStringASCII(description)) {
    LOG(ERROR) << "The accelerator string can only contain ASCII characters";
    return false;
  }
  std::string shortcut(base::StringToLowerASCII(description));

  std::vector<std::string> tokens;
  base::SplitString(shortcut, '+', &tokens);

  // Now, parse it into an accelerator.
  int modifiers = ui::EF_NONE;
  ui::KeyboardCode key = ui::VKEY_UNKNOWN;
  for (size_t i = 0; i < tokens.size(); i++) {
    // We use straight comparing instead of map because the accelerator tends
    // to be correct and usually only uses few special tokens.
    if (tokens[i].size() == 1) {
      bool shifted = false;
      key = KeyboardCodeFromCharCode(tokens[i][0], &shifted);
      if (shifted)
        modifiers |= ui::EF_SHIFT_DOWN;
    } else if (tokens[i] == "ctrl" || tokens[i] == "control") {
      modifiers |= ui::EF_CONTROL_DOWN;
    } else if (tokens[i] == "cmd" || tokens[i] == "command") {
      modifiers |= ui::EF_COMMAND_DOWN;
    } else if (tokens[i] == "commandorcontrol" || tokens[i] == "cmdorctrl") {
#if defined(OS_MACOSX)
      modifiers |= ui::EF_COMMAND_DOWN;
#else
      modifiers |= ui::EF_CONTROL_DOWN;
#endif
    } else if (tokens[i] == "alt") {
      modifiers |= ui::EF_ALT_DOWN;
    } else if (tokens[i] == "shift") {
      modifiers |= ui::EF_SHIFT_DOWN;
    } else if (tokens[i] == "plus") {
      modifiers |= ui::EF_SHIFT_DOWN;
      key = ui::VKEY_OEM_PLUS;
    } else if (tokens[i] == "tab") {
      key = ui::VKEY_TAB;
    } else if (tokens[i] == "space") {
      key = ui::VKEY_SPACE;
    } else if (tokens[i] == "backspace") {
      key = ui::VKEY_BACK;
    } else if (tokens[i] == "delete") {
      key = ui::VKEY_DELETE;
    } else if (tokens[i] == "insert") {
      key = ui::VKEY_INSERT;
    } else if (tokens[i] == "enter" || tokens[i] == "return") {
      key = ui::VKEY_RETURN;
    } else if (tokens[i] == "up") {
      key = ui::VKEY_UP;
    } else if (tokens[i] == "down") {
      key = ui::VKEY_DOWN;
    } else if (tokens[i] == "left") {
      key = ui::VKEY_LEFT;
    } else if (tokens[i] == "right") {
      key = ui::VKEY_RIGHT;
    } else if (tokens[i] == "home") {
      key = ui::VKEY_HOME;
    } else if (tokens[i] == "end") {
      key = ui::VKEY_END;
    } else if (tokens[i] == "pagedown") {
      key = ui::VKEY_PRIOR;
    } else if (tokens[i] == "pageup") {
      key = ui::VKEY_NEXT;
    } else if (tokens[i] == "esc" || tokens[i] == "escape") {
      key = ui::VKEY_ESCAPE;
    } else if (tokens[i] == "volumemute") {
      key = ui::VKEY_VOLUME_MUTE;
    } else if (tokens[i] == "volumeup") {
      key = ui::VKEY_VOLUME_UP;
    } else if (tokens[i] == "volumedown") {
      key = ui::VKEY_VOLUME_DOWN;
    } else if (tokens[i] == "medianexttrack") {
      key = ui::VKEY_MEDIA_NEXT_TRACK;
    } else if (tokens[i] == "mediaprevioustrack") {
      key = ui::VKEY_MEDIA_PREV_TRACK;
    } else if (tokens[i] == "mediastop") {
      key = ui::VKEY_MEDIA_STOP;
    } else if (tokens[i] == "mediaplaypause") {
      key = ui::VKEY_MEDIA_PLAY_PAUSE;
    } else if (tokens[i].size() > 1 && tokens[i][0] == 'f') {
      // F1 - F24.
      int n;
      if (base::StringToInt(tokens[i].c_str() + 1, &n)) {
        key = static_cast<ui::KeyboardCode>(ui::VKEY_F1 + n - 1);
      } else {
        LOG(WARNING) << tokens[i] << "is not available on keyboard";
        return false;
      }
    } else {
      LOG(WARNING) << "Invalid accelerator token: " << tokens[i];
      return false;
    }
  }

  if (key == ui::VKEY_UNKNOWN) {
    LOG(WARNING) << "The accelerator doesn't contain a valid key";
    return false;
  }

  *accelerator = ui::Accelerator(key, modifiers);
  SetPlatformAccelerator(accelerator);
  return true;
}

void GenerateAcceleratorTable(AcceleratorTable* table, ui::MenuModel* model) {
  int count = model->GetItemCount();
  for (int i = 0; i < count; ++i) {
    ui::MenuModel::ItemType type = model->GetTypeAt(i);
    if (type == ui::MenuModel::TYPE_SUBMENU) {
      ui::MenuModel* submodel = model->GetSubmenuModelAt(i);
      GenerateAcceleratorTable(table, submodel);
    } else {
      ui::Accelerator accelerator;
      if (model->GetAcceleratorAt(i, &accelerator)) {
        MenuItem item = { i, model };
        (*table)[accelerator] = item;
      }
    }
  }
}

bool TriggerAcceleratorTableCommand(AcceleratorTable* table,
                                    const ui::Accelerator& accelerator) {
  if (ContainsKey(*table, accelerator)) {
    const accelerator_util::MenuItem& item = (*table)[accelerator];
    item.model->ActivatedAt(item.position);
    return true;
  } else {
    return false;
  }
}

}  // namespace accelerator_util
