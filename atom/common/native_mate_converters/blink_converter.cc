// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/blink_converter.h"

#include <string>
#include <vector>

#include "atom/common/keyboard_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "native_mate/dictionary.h"
#include "third_party/WebKit/public/web/WebDeviceEmulationParams.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"
#include "ui/base/clipboard/clipboard.h"

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
    std::string type = base::ToLowerASCII(V8ToString(val));
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
    std::string button = base::ToLowerASCII(V8ToString(val));
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
    std::string modifier = base::ToLowerASCII(V8ToString(val));
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

  std::string str;
  bool shifted = false;
  if (dict.Get("keyCode", &str))
    out->windowsKeyCode = atom::KeyboardCodeFromStr(str, &shifted);
  else
    return false;

  if (shifted)
    out->modifiers |= blink::WebInputEvent::ShiftKey;
  out->setKeyIdentifierFromWindowsKeyCode();
  if ((out->type == blink::WebInputEvent::Char ||
       out->type == blink::WebInputEvent::RawKeyDown) &&
      str.size() == 1) {
    out->text[0] = str[0];
    out->unmodifiedText[0] = str[0];
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
    screen_position = base::ToLowerASCII(screen_position);
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

bool Converter<blink::WebFindOptions>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::WebFindOptions* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  dict.Get("forward", &out->forward);
  dict.Get("matchCase", &out->matchCase);
  dict.Get("findNext", &out->findNext);
  dict.Get("wordStart", &out->wordStart);
  dict.Get("medialCapitalAsWordStart", &out->medialCapitalAsWordStart);
  return true;
}

// static
v8::Local<v8::Value> Converter<blink::WebContextMenuData::MediaType>::ToV8(
      v8::Isolate* isolate, const blink::WebContextMenuData::MediaType& in) {
  switch (in) {
    case blink::WebContextMenuData::MediaTypeImage:
      return mate::StringToV8(isolate, "image");
    case blink::WebContextMenuData::MediaTypeVideo:
      return mate::StringToV8(isolate, "video");
    case blink::WebContextMenuData::MediaTypeAudio:
      return mate::StringToV8(isolate, "audio");
    case blink::WebContextMenuData::MediaTypeCanvas:
      return mate::StringToV8(isolate, "canvas");
    case blink::WebContextMenuData::MediaTypeFile:
      return mate::StringToV8(isolate, "file");
    case blink::WebContextMenuData::MediaTypePlugin:
      return mate::StringToV8(isolate, "plugin");
    default:
      return mate::StringToV8(isolate, "none");
  }
}

// static
v8::Local<v8::Value> Converter<blink::WebContextMenuData::InputFieldType>::ToV8(
      v8::Isolate* isolate,
      const blink::WebContextMenuData::InputFieldType& in) {
  switch (in) {
    case blink::WebContextMenuData::InputFieldTypePlainText:
      return mate::StringToV8(isolate, "plainText");
    case blink::WebContextMenuData::InputFieldTypePassword:
      return mate::StringToV8(isolate, "password");
    case blink::WebContextMenuData::InputFieldTypeOther:
      return mate::StringToV8(isolate, "other");
    default:
      return mate::StringToV8(isolate, "none");
  }
}

v8::Local<v8::Value> EditFlagsToV8(v8::Isolate* isolate, int editFlags) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("canUndo",
      !!(editFlags & blink::WebContextMenuData::CanUndo));
  dict.Set("canRedo",
      !!(editFlags & blink::WebContextMenuData::CanRedo));
  dict.Set("canCut",
      !!(editFlags & blink::WebContextMenuData::CanCut));
  dict.Set("canCopy",
      !!(editFlags & blink::WebContextMenuData::CanCopy));

  bool pasteFlag = false;
  if (editFlags & blink::WebContextMenuData::CanPaste) {
    std::vector<base::string16> types;
    bool ignore;
    ui::Clipboard::GetForCurrentThread()->ReadAvailableTypes(
        ui::CLIPBOARD_TYPE_COPY_PASTE, &types, &ignore);
    pasteFlag = !types.empty();
  }
  dict.Set("canPaste", pasteFlag);

  dict.Set("canDelete",
      !!(editFlags & blink::WebContextMenuData::CanDelete));
  dict.Set("canSelectAll",
      !!(editFlags & blink::WebContextMenuData::CanSelectAll));

  return mate::ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("inError",
      !!(mediaFlags & blink::WebContextMenuData::MediaInError));
  dict.Set("isPaused",
      !!(mediaFlags & blink::WebContextMenuData::MediaPaused));
  dict.Set("isMuted",
      !!(mediaFlags & blink::WebContextMenuData::MediaMuted));
  dict.Set("hasAudio",
      !!(mediaFlags & blink::WebContextMenuData::MediaHasAudio));
  dict.Set("isLooping",
      (mediaFlags & blink::WebContextMenuData::MediaLoop) != 0);
  dict.Set("isControlsVisible",
      (mediaFlags & blink::WebContextMenuData::MediaControls) != 0);
  dict.Set("canToggleControls",
      !!(mediaFlags & blink::WebContextMenuData::MediaCanToggleControls));
  dict.Set("canRotate",
      !!(mediaFlags & blink::WebContextMenuData::MediaCanRotate));
  return mate::ConvertToV8(isolate, dict);
}

}  // namespace mate
