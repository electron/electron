// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "atom/common/keyboard_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace atom {

namespace {

// Return key code of the char, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromCharCode(base::char16 c, bool* shifted) {
  c = base::ToLowerASCII(c);
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

// Return key code represented by |str|.
ui::KeyboardCode KeyboardCodeFromKeyIdentifier(const std::string& s,
                                               bool* shifted) {
  std::string str = base::ToLowerASCII(s);
  if (str == "ctrl" || str == "control") {
    return ui::VKEY_CONTROL;
  } else if (str == "super" || str == "cmd" || str == "command" ||
             str == "meta") {
    return ui::VKEY_COMMAND;
  } else if (str == "commandorcontrol" || str == "cmdorctrl") {
#if defined(OS_MACOSX)
    return ui::VKEY_COMMAND;
#else
    return ui::VKEY_CONTROL;
#endif
  } else if (str == "alt" || str == "option") {
    return ui::VKEY_MENU;
  } else if (str == "shift") {
    return ui::VKEY_SHIFT;
  } else if (str == "altgr") {
    return ui::VKEY_ALTGR;
  } else if (str == "plus") {
    *shifted = true;
    return ui::VKEY_OEM_PLUS;
  } else if (str == "tab") {
    return ui::VKEY_TAB;
  } else if (str == "space") {
    return ui::VKEY_SPACE;
  } else if (str == "backspace") {
    return ui::VKEY_BACK;
  } else if (str == "delete") {
    return ui::VKEY_DELETE;
  } else if (str == "insert") {
    return ui::VKEY_INSERT;
  } else if (str == "enter" || str == "return") {
    return ui::VKEY_RETURN;
  } else if (str == "up") {
    return ui::VKEY_UP;
  } else if (str == "down") {
    return ui::VKEY_DOWN;
  } else if (str == "left") {
    return ui::VKEY_LEFT;
  } else if (str == "right") {
    return ui::VKEY_RIGHT;
  } else if (str == "home") {
    return ui::VKEY_HOME;
  } else if (str == "end") {
    return ui::VKEY_END;
  } else if (str == "pageup") {
    return ui::VKEY_PRIOR;
  } else if (str == "pagedown") {
    return ui::VKEY_NEXT;
  } else if (str == "esc" || str == "escape") {
    return ui::VKEY_ESCAPE;
  } else if (str == "volumemute") {
    return ui::VKEY_VOLUME_MUTE;
  } else if (str == "volumeup") {
    return ui::VKEY_VOLUME_UP;
  } else if (str == "volumedown") {
    return ui::VKEY_VOLUME_DOWN;
  } else if (str == "medianexttrack") {
    return ui::VKEY_MEDIA_NEXT_TRACK;
  } else if (str == "mediaprevioustrack") {
    return ui::VKEY_MEDIA_PREV_TRACK;
  } else if (str == "mediastop") {
    return ui::VKEY_MEDIA_STOP;
  } else if (str == "mediaplaypause") {
    return ui::VKEY_MEDIA_PLAY_PAUSE;
  } else if (str == "printscreen") {
    return ui::VKEY_SNAPSHOT;
  } else if (str.size() > 1 && str[0] == 'f') {
    // F1 - F24.
    int n;
    if (base::StringToInt(str.c_str() + 1, &n) && n > 0 && n < 25) {
      return static_cast<ui::KeyboardCode>(ui::VKEY_F1 + n - 1);
    } else {
      LOG(WARNING) << str << "is not available on keyboard";
      return ui::VKEY_UNKNOWN;
    }
  } else {
    LOG(WARNING) << "Invalid accelerator token: " << str;
    return ui::VKEY_UNKNOWN;
  }
}

}  // namespace

ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted) {
  if (str.size() == 1)
    return KeyboardCodeFromCharCode(str[0], shifted);
  else
    return KeyboardCodeFromKeyIdentifier(str, shifted);
}

}  // namespace atom
