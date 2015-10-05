// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/blink_converter.h"

#include <string>
#include <vector>

#include "atom/common/keyboad_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebDeviceEmulationParams.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

namespace {

template<typename T>
int VectorToBitArray(const std::vector<T>& vec) {
  int bits = 0;
  for (const T& item : vec)
    bits |= item;
  return bits;
}

}  // namespace

namespace mate {

template<>
struct Converter<base::char16> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     base::char16* out) {
    base::string16 code = base::UTF8ToUTF16(V8ToString(val));
    if (code.length() != 1)
      return false;
    *out = code[0];
    return true;
  }
};

template<>
struct Converter<blink::WebInputEvent::Type> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     blink::WebInputEvent::Type* out) {
    std::string type = base::StringToLowerASCII(V8ToString(val));
    if (type == "mousedown")
      *out = blink::WebInputEvent::MouseDown;
    else if (type == "mouseup")
      *out = blink::WebInputEvent::MouseUp;
    else if (type == "mousemove")
      *out = blink::WebInputEvent::MouseMove;
    else if (type == "mouseenter")
      *out = blink::WebInputEvent::MouseEnter;
    else if (type == "mouseleave")
      *out = blink::WebInputEvent::MouseLeave;
    else if (type == "contextmenu")
      *out = blink::WebInputEvent::ContextMenu;
    else if (type == "mousewheel")
      *out = blink::WebInputEvent::MouseWheel;
    else if (type == "keydown")
      *out = blink::WebInputEvent::RawKeyDown;
    else if (type == "keyup")
      *out = blink::WebInputEvent::KeyUp;
    else if (type == "char")
      *out = blink::WebInputEvent::Char;
    else if (type == "touchstart")
      *out = blink::WebInputEvent::TouchStart;
    else if (type == "touchmove")
      *out = blink::WebInputEvent::TouchMove;
    else if (type == "touchend")
      *out = blink::WebInputEvent::TouchEnd;
    else if (type == "touchcancel")
      *out = blink::WebInputEvent::TouchCancel;
    return true;
  }
};

template<>
struct Converter<blink::WebMouseEvent::Button> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     blink::WebMouseEvent::Button* out) {
    std::string button = base::StringToLowerASCII(V8ToString(val));
    if (button == "left")
      *out = blink::WebMouseEvent::Button::ButtonLeft;
    else if (button == "middle")
      *out = blink::WebMouseEvent::Button::ButtonMiddle;
    else if (button == "right")
      *out = blink::WebMouseEvent::Button::ButtonRight;
    return true;
  }
};

template<>
struct Converter<blink::WebInputEvent::Modifiers> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     blink::WebInputEvent::Modifiers* out) {
    std::string modifier = base::StringToLowerASCII(V8ToString(val));
    if (modifier == "shift")
      *out = blink::WebInputEvent::ShiftKey;
    else if (modifier == "control" || modifier == "ctrl")
      *out = blink::WebInputEvent::ControlKey;
    else if (modifier == "alt")
      *out = blink::WebInputEvent::AltKey;
    else if (modifier == "meta" || modifier == "command" || modifier == "cmd")
      *out = blink::WebInputEvent::MetaKey;
    else if (modifier == "iskeypad")
      *out = blink::WebInputEvent::IsKeyPad;
    else if (modifier == "isautorepeat")
      *out = blink::WebInputEvent::IsAutoRepeat;
    else if (modifier == "leftbuttondown")
      *out = blink::WebInputEvent::LeftButtonDown;
    else if (modifier == "middlebuttondown")
      *out = blink::WebInputEvent::MiddleButtonDown;
    else if (modifier == "rightbuttondown")
      *out = blink::WebInputEvent::RightButtonDown;
    else if (modifier == "capslock")
      *out = blink::WebInputEvent::CapsLockOn;
    else if (modifier == "numlock")
      *out = blink::WebInputEvent::NumLockOn;
    else if (modifier == "left")
      *out = blink::WebInputEvent::IsLeft;
    else if (modifier == "right")
      *out = blink::WebInputEvent::IsRight;
    return true;
  }
};

int GetWebInputEventType(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  blink::WebInputEvent::Type type = blink::WebInputEvent::Undefined;
  mate::Dictionary dict;
  ConvertFromV8(isolate, val, &dict) && dict.Get("type", &type);
  return type;
}

bool Converter<blink::WebInputEvent>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    blink::WebInputEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!dict.Get("type", &out->type))
    return false;
  std::vector<blink::WebInputEvent::Modifiers> modifiers;
  if (dict.Get("modifiers", &modifiers))
    out->modifiers = VectorToBitArray(modifiers);
  out->timeStampSeconds = base::Time::Now().ToDoubleT();
  return true;
}

bool Converter<blink::WebKeyboardEvent>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    blink::WebKeyboardEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebInputEvent*>(out)))
    return false;
  base::char16 code;
  if (!dict.Get("keyCode", &code))
    return false;
  bool shifted = false;
  out->windowsKeyCode = atom::KeyboardCodeFromCharCode(code, &shifted);
  if (shifted)
    out->modifiers |= blink::WebInputEvent::ShiftKey;
  out->setKeyIdentifierFromWindowsKeyCode();
  if (out->type == blink::WebInputEvent::Char ||
      out->type == blink::WebInputEvent::RawKeyDown) {
    out->text[0] = code;
    out->unmodifiedText[0] = code;
  }
  return true;
}

bool Converter<content::NativeWebKeyboardEvent>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    content::NativeWebKeyboardEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebKeyboardEvent*>(out)))
    return false;
  dict.Get("skipInBrowser", &out->skip_in_browser);
  return true;
}

bool Converter<blink::WebMouseEvent>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebMouseEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebInputEvent*>(out)))
    return false;
  if (!dict.Get("x", &out->x) || !dict.Get("y", &out->y))
    return false;
  dict.Get("button", &out->button);
  dict.Get("globalX", &out->globalX);
  dict.Get("globalY", &out->globalY);
  dict.Get("movementX", &out->movementX);
  dict.Get("movementY", &out->movementY);
  dict.Get("clickCount", &out->clickCount);
  return true;
}

bool Converter<blink::WebMouseWheelEvent>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    blink::WebMouseWheelEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebMouseEvent*>(out)))
    return false;
  dict.Get("deltaX", &out->deltaX);
  dict.Get("deltaY", &out->deltaY);
  dict.Get("wheelTicksX", &out->wheelTicksX);
  dict.Get("wheelTicksY", &out->wheelTicksY);
  dict.Get("accelerationRatioX", &out->accelerationRatioX);
  dict.Get("accelerationRatioY", &out->accelerationRatioY);
  dict.Get("hasPreciseScrollingDeltas", &out->hasPreciseScrollingDeltas);
  dict.Get("canScroll", &out->canScroll);
  return true;
}

bool Converter<blink::WebFloatPoint>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebFloatPoint* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("x", &out->x) && dict.Get("y", &out->y);
}

bool Converter<blink::WebPoint>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebPoint* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("x", &out->x) && dict.Get("y", &out->y);
}

bool Converter<blink::WebSize>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val, blink::WebSize* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("width", &out->width) && dict.Get("height", &out->height);
}

bool Converter<blink::WebDeviceEmulationParams>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    blink::WebDeviceEmulationParams* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  std::string screen_position;
  if (dict.Get("screenPosition", &screen_position)) {
    screen_position = base::StringToLowerASCII(screen_position);
    if (screen_position == "mobile")
      out->screenPosition = blink::WebDeviceEmulationParams::Mobile;
    else if (screen_position == "desktop")
      out->screenPosition = blink::WebDeviceEmulationParams::Desktop;
    else
      return false;
  }

  dict.Get("screenSize", &out->screenSize);
  dict.Get("viewPosition", &out->viewPosition);
  dict.Get("deviceScaleFactor", &out->deviceScaleFactor);
  dict.Get("viewSize", &out->viewSize);
  dict.Get("fitToView", &out->fitToView);
  dict.Get("offset", &out->offset);
  dict.Get("scale", &out->scale);
  return true;
}

}  // namespace mate
