// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "shell/common/keyboard_util.h"
#include "shell/common/string_lookup.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "ui/events/event_constants.h"

namespace electron {

namespace {

#if defined(OS_MAC)
#define VKEY_CONTROL_OR_COMMAND ui::VKEY_COMMAND
#else
#define VKEY_CONTROL_OR_COMMAND ui::VKEY_CONTROL
#endif

// Return key code represented by |str|.
ui::KeyboardCode KeyboardCodeFromKeyIdentifier(const std::string& s,
                                               bool* shifted) {
  static constexpr StringLookup<ui::KeyboardCode, 80> keymap{
      {{{"alt", ui::VKEY_MENU},
        {"altgr", ui::VKEY_ALTGR},
        {"backspace", ui::VKEY_BACK},
        {"capslock", ui::VKEY_CAPITAL},
        {"cmd", ui::VKEY_COMMAND},
        {"cmdorctrl", VKEY_CONTROL_OR_COMMAND},
        {"command", ui::VKEY_COMMAND},
        {"commandorcontrol", VKEY_CONTROL_OR_COMMAND},
        {"control", ui::VKEY_CONTROL},
        {"ctrl", ui::VKEY_CONTROL},
        {"delete", ui::VKEY_DELETE},
        {"down", ui::VKEY_DOWN},
        {"end", ui::VKEY_END},
        {"enter", ui::VKEY_RETURN},
        {"esc", ui::VKEY_ESCAPE},
        {"escape", ui::VKEY_ESCAPE},
        {"f1", ui::VKEY_F1},
        {"f10", ui::VKEY_F10},
        {"f11", ui::VKEY_F11},
        {"f12", ui::VKEY_F12},
        {"f13", ui::VKEY_F13},
        {"f14", ui::VKEY_F14},
        {"f15", ui::VKEY_F15},
        {"f16", ui::VKEY_F16},
        {"f17", ui::VKEY_F17},
        {"f18", ui::VKEY_F18},
        {"f19", ui::VKEY_F19},
        {"f2", ui::VKEY_F2},
        {"f20", ui::VKEY_F20},
        {"f21", ui::VKEY_F21},
        {"f22", ui::VKEY_F22},
        {"f23", ui::VKEY_F23},
        {"f24", ui::VKEY_F24},
        {"f3", ui::VKEY_F3},
        {"f4", ui::VKEY_F4},
        {"f5", ui::VKEY_F5},
        {"f6", ui::VKEY_F6},
        {"f7", ui::VKEY_F7},
        {"f8", ui::VKEY_F8},
        {"f9", ui::VKEY_F9},
        {"home", ui::VKEY_HOME},
        {"insert", ui::VKEY_INSERT},
        {"left", ui::VKEY_LEFT},
        {"medianexttrack", ui::VKEY_MEDIA_NEXT_TRACK},
        {"mediaplaypause", ui::VKEY_MEDIA_PLAY_PAUSE},
        {"mediaprevioustrack", ui::VKEY_MEDIA_PREV_TRACK},
        {"mediastop", ui::VKEY_MEDIA_STOP},
        {"meta", ui::VKEY_COMMAND},
        {"num0", ui::VKEY_NUMPAD0},
        {"num1", ui::VKEY_NUMPAD1},
        {"num2", ui::VKEY_NUMPAD2},
        {"num3", ui::VKEY_NUMPAD3},
        {"num4", ui::VKEY_NUMPAD4},
        {"num5", ui::VKEY_NUMPAD5},
        {"num6", ui::VKEY_NUMPAD6},
        {"num7", ui::VKEY_NUMPAD7},
        {"num8", ui::VKEY_NUMPAD8},
        {"num9", ui::VKEY_NUMPAD9},
        {"numadd", ui::VKEY_ADD},
        {"numdec", ui::VKEY_DECIMAL},
        {"numdiv", ui::VKEY_DIVIDE},
        {"numlock", ui::VKEY_NUMLOCK},
        {"nummult", ui::VKEY_MULTIPLY},
        {"numsub", ui::VKEY_SUBTRACT},
        {"option", ui::VKEY_MENU},
        {"pagedown", ui::VKEY_NEXT},
        {"pageup", ui::VKEY_PRIOR},
        {"plus", ui::VKEY_OEM_PLUS},
        {"printscreen", ui::VKEY_SNAPSHOT},
        {"return", ui::VKEY_RETURN},
        {"right", ui::VKEY_RIGHT},
        {"scrolllock", ui::VKEY_SCROLL},
        {"shift", ui::VKEY_SHIFT},
        {"space", ui::VKEY_SPACE},
        {"super", ui::VKEY_COMMAND},
        {"tab", ui::VKEY_TAB},
        {"up", ui::VKEY_UP},
        {"volumedown", ui::VKEY_VOLUME_DOWN},
        {"volumemute", ui::VKEY_VOLUME_MUTE},
        {"volumeup", ui::VKEY_VOLUME_UP}}}};
  static_assert(keymap.IsSorted(), "please sort keymap");

  auto const keycode = keymap.lookup(base::ToLowerASCII(s));
  if (!keycode) {
    if (s.size() > 2)
      LOG(WARNING) << "Invalid accelerator token: " << s;
    return ui::VKEY_UNKNOWN;
  }

  if (*keycode == ui::VKEY_OEM_PLUS) {
    *shifted = true;
  }

  return *keycode;
}

}  // namespace

ui::KeyboardCode KeyboardCodeFromCharCode(base::char16 c, bool* shifted) {
  c = base::ToLowerASCII(c);
  *shifted = false;
  switch (c) {
    case 0x08:
      return ui::VKEY_BACK;
    case 0x7F:
      return ui::VKEY_DELETE;
    case 0x09:
      return ui::VKEY_TAB;
    case 0x0D:
      return ui::VKEY_RETURN;
    case 0x1B:
      return ui::VKEY_ESCAPE;
    case ' ':
      return ui::VKEY_SPACE;
    case 'a':
      return ui::VKEY_A;
    case 'b':
      return ui::VKEY_B;
    case 'c':
      return ui::VKEY_C;
    case 'd':
      return ui::VKEY_D;
    case 'e':
      return ui::VKEY_E;
    case 'f':
      return ui::VKEY_F;
    case 'g':
      return ui::VKEY_G;
    case 'h':
      return ui::VKEY_H;
    case 'i':
      return ui::VKEY_I;
    case 'j':
      return ui::VKEY_J;
    case 'k':
      return ui::VKEY_K;
    case 'l':
      return ui::VKEY_L;
    case 'm':
      return ui::VKEY_M;
    case 'n':
      return ui::VKEY_N;
    case 'o':
      return ui::VKEY_O;
    case 'p':
      return ui::VKEY_P;
    case 'q':
      return ui::VKEY_Q;
    case 'r':
      return ui::VKEY_R;
    case 's':
      return ui::VKEY_S;
    case 't':
      return ui::VKEY_T;
    case 'u':
      return ui::VKEY_U;
    case 'v':
      return ui::VKEY_V;
    case 'w':
      return ui::VKEY_W;
    case 'x':
      return ui::VKEY_X;
    case 'y':
      return ui::VKEY_Y;
    case 'z':
      return ui::VKEY_Z;

    case ')':
      *shifted = true;
      FALLTHROUGH;
    case '0':
      return ui::VKEY_0;
    case '!':
      *shifted = true;
      FALLTHROUGH;
    case '1':
      return ui::VKEY_1;
    case '@':
      *shifted = true;
      FALLTHROUGH;
    case '2':
      return ui::VKEY_2;
    case '#':
      *shifted = true;
      FALLTHROUGH;
    case '3':
      return ui::VKEY_3;
    case '$':
      *shifted = true;
      FALLTHROUGH;
    case '4':
      return ui::VKEY_4;
    case '%':
      *shifted = true;
      FALLTHROUGH;
    case '5':
      return ui::VKEY_5;
    case '^':
      *shifted = true;
      FALLTHROUGH;
    case '6':
      return ui::VKEY_6;
    case '&':
      *shifted = true;
      FALLTHROUGH;
    case '7':
      return ui::VKEY_7;
    case '*':
      *shifted = true;
      FALLTHROUGH;
    case '8':
      return ui::VKEY_8;
    case '(':
      *shifted = true;
      FALLTHROUGH;
    case '9':
      return ui::VKEY_9;

    case ':':
      *shifted = true;
      FALLTHROUGH;
    case ';':
      return ui::VKEY_OEM_1;
    case '+':
      *shifted = true;
      FALLTHROUGH;
    case '=':
      return ui::VKEY_OEM_PLUS;
    case '<':
      *shifted = true;
      FALLTHROUGH;
    case ',':
      return ui::VKEY_OEM_COMMA;
    case '_':
      *shifted = true;
      FALLTHROUGH;
    case '-':
      return ui::VKEY_OEM_MINUS;
    case '>':
      *shifted = true;
      FALLTHROUGH;
    case '.':
      return ui::VKEY_OEM_PERIOD;
    case '?':
      *shifted = true;
      FALLTHROUGH;
    case '/':
      return ui::VKEY_OEM_2;
    case '~':
      *shifted = true;
      FALLTHROUGH;
    case '`':
      return ui::VKEY_OEM_3;
    case '{':
      *shifted = true;
      FALLTHROUGH;
    case '[':
      return ui::VKEY_OEM_4;
    case '|':
      *shifted = true;
      FALLTHROUGH;
    case '\\':
      return ui::VKEY_OEM_5;
    case '}':
      *shifted = true;
      FALLTHROUGH;
    case ']':
      return ui::VKEY_OEM_6;
    case '"':
      *shifted = true;
      FALLTHROUGH;
    case '\'':
      return ui::VKEY_OEM_7;

    default:
      return ui::VKEY_UNKNOWN;
  }
}

ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted) {
  if (str.size() == 1)
    return KeyboardCodeFromCharCode(str[0], shifted);
  else
    return KeyboardCodeFromKeyIdentifier(str, shifted);
}

int WebEventModifiersToEventFlags(int modifiers) {
  int flags = 0;

  if (modifiers & blink::WebInputEvent::Modifiers::kShiftKey)
    flags |= ui::EF_SHIFT_DOWN;
  if (modifiers & blink::WebInputEvent::Modifiers::kControlKey)
    flags |= ui::EF_CONTROL_DOWN;
  if (modifiers & blink::WebInputEvent::Modifiers::kAltKey)
    flags |= ui::EF_ALT_DOWN;
  if (modifiers & blink::WebInputEvent::Modifiers::kMetaKey)
    flags |= ui::EF_COMMAND_DOWN;
  if (modifiers & blink::WebInputEvent::Modifiers::kCapsLockOn)
    flags |= ui::EF_CAPS_LOCK_ON;
  if (modifiers & blink::WebInputEvent::Modifiers::kNumLockOn)
    flags |= ui::EF_NUM_LOCK_ON;
  if (modifiers & blink::WebInputEvent::Modifiers::kScrollLockOn)
    flags |= ui::EF_SCROLL_LOCK_ON;
  if (modifiers & blink::WebInputEvent::Modifiers::kLeftButtonDown)
    flags |= ui::EF_LEFT_MOUSE_BUTTON;
  if (modifiers & blink::WebInputEvent::Modifiers::kMiddleButtonDown)
    flags |= ui::EF_MIDDLE_MOUSE_BUTTON;
  if (modifiers & blink::WebInputEvent::Modifiers::kRightButtonDown)
    flags |= ui::EF_RIGHT_MOUSE_BUTTON;
  if (modifiers & blink::WebInputEvent::Modifiers::kIsAutoRepeat)
    flags |= ui::EF_IS_REPEAT;

  return flags;
}

}  // namespace electron
