// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/event_types.h"

namespace atom {

namespace event_types {

const char kMouseDown[] = "down";
const char kMouseUp[] = "up";
const char kMouseMove[] = "move";
const char kMouseEnter[] = "enter";
const char kMouseLeave[] = "leave";
const char kContextMenu[] = "context-menu";
const char kMouseWheel[] = "wheel";

const char kKeyDown[] = "key-down";
const char kKeyUp[] = "key-up";
const char kChar[] = "char";

const char kMouseLeftButton[] = "left";
const char kMouseRightButton[] = "right";
const char kMouseMiddleButton[] = "middle";

const char kModifierLeftButtonDown[] = "left-button-down";
const char kModifierMiddleButtonDown[] = "middle-button-down";
const char kModifierRightButtonDown[] = "right-button-down";

const char kModifierShiftKey[] = "shift";
const char kModifierControlKey[] = "control";
const char kModifierAltKey[] = "alt";
const char kModifierMetaKey[] = "meta";
const char kModifierCapsLockOn[] = "caps-lock";
const char kModifierNumLockOn[] = "num-lock";

const char kModifierIsKeyPad[] = "keypad";
const char kModifierIsAutoRepeat[] = "auto-repeat";
const char kModifierIsLeft[] = "left";
const char kModifierIsRight[] = "right";

}  // namespace switches

}  // namespace atom
