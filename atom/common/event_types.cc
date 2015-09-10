// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/event_types.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

namespace atom {

namespace event_types {

const char kMouseDown[] = "down";
const char kMouseUp[] = "up";
const char kMouseMove[] = "move";
const char kMouseEnter[] = "enter";
const char kMouseLeave[] = "leave";
const char kContextMenu[] = "context-menu";
const char kMouseWheel[] = "wheel";

const char kKeyDown[] = "down";
const char kKeyUp[] = "up";
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

int modifierStrToWebModifier(std::string modifier){
  if(modifier.compare(event_types::kModifierLeftButtonDown) == 0){

    return blink::WebInputEvent::Modifiers::LeftButtonDown;

  }else if(modifier.compare(event_types::kModifierMiddleButtonDown) == 0){

    return blink::WebInputEvent::Modifiers::MiddleButtonDown;

  }else if(modifier.compare(event_types::kModifierRightButtonDown) == 0){

    return blink::WebInputEvent::Modifiers::RightButtonDown;

  }else if(modifier.compare(event_types::kMouseLeftButton) == 0){

    return blink::WebInputEvent::Modifiers::LeftButtonDown;

  }else if(modifier.compare(event_types::kMouseRightButton) == 0){

    return blink::WebInputEvent::Modifiers::RightButtonDown;

  }else if(modifier.compare(event_types::kMouseMiddleButton) == 0){

    return blink::WebInputEvent::Modifiers::MiddleButtonDown;

  }else if(modifier.compare(event_types::kModifierIsKeyPad) == 0){

    return blink::WebInputEvent::Modifiers::IsKeyPad;

  }else if(modifier.compare(event_types::kModifierIsAutoRepeat) == 0){

    return blink::WebInputEvent::Modifiers::IsAutoRepeat;

  }else if(modifier.compare(event_types::kModifierIsLeft) == 0){

    return blink::WebInputEvent::Modifiers::IsLeft;

  }else if(modifier.compare(event_types::kModifierIsRight) == 0){

    return blink::WebInputEvent::Modifiers::IsRight;

  }else if(modifier.compare(event_types::kModifierShiftKey) == 0){

    return blink::WebInputEvent::Modifiers::ShiftKey;

  }else if(modifier.compare(event_types::kModifierControlKey) == 0){

    return blink::WebInputEvent::Modifiers::ControlKey;

  }else if(modifier.compare(event_types::kModifierAltKey) == 0){

    return blink::WebInputEvent::Modifiers::AltKey;

  }else if(modifier.compare(event_types::kModifierMetaKey) == 0){

    return blink::WebInputEvent::Modifiers::MetaKey;

  }else if(modifier.compare(event_types::kModifierCapsLockOn) == 0){

    return blink::WebInputEvent::Modifiers::CapsLockOn;

  }else if(modifier.compare(event_types::kModifierNumLockOn) == 0){

    return blink::WebInputEvent::Modifiers::NumLockOn;

  }else{

    return 0;

  }
}

}  // namespace event_types

}  // namespace atom
