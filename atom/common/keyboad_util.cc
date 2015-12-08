// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include "atom/common/keyboad_util.h"

namespace atom {

// Return key code of the char.
ui::KeyboardCode KeyboardCodeFromCharCode(base::char16 c, bool* shifted) {
  *shifted = false;
  switch (c) {
    case 0x08: return ui::VKEY_BACK;
    case 0x7F: return ui::VKEY_DELETE;
    case 0x09: return ui::VKEY_TAB;
    case 0x0D: return ui::VKEY_RETURN;
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

// Return key code of the char.
ui::KeyboardCode KeyboardCodeFromKeyIdentifier(const std::string& chr) {
  if (chr == "enter")        return ui::VKEY_RETURN;
  if (chr == "backspace")    return ui::VKEY_BACK;
  if (chr == "delete")       return ui::VKEY_DELETE;
  if (chr == "tab")          return ui::VKEY_TAB;
  if (chr == "escape")       return ui::VKEY_ESCAPE;
  if (chr == "control")      return ui::VKEY_CONTROL;
  if (chr == "alt")          return ui::VKEY_MENU;
  if (chr == "shift")        return ui::VKEY_SHIFT;
  if (chr == "end")          return ui::VKEY_END;
  if (chr == "home")         return ui::VKEY_HOME;
  if (chr == "insert")       return ui::VKEY_INSERT;
  if (chr == "left")         return ui::VKEY_LEFT;
  if (chr == "up")           return ui::VKEY_UP;
  if (chr == "right")        return ui::VKEY_RIGHT;
  if (chr == "down")         return ui::VKEY_DOWN;
  if (chr == "pageup")       return ui::VKEY_PRIOR;
  if (chr == "pagedown")     return ui::VKEY_NEXT;
  if (chr == "printscreen")  return ui::VKEY_SNAPSHOT;

  return ui::VKEY_UNKNOWN;
}

}  // namespace atom
