// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/blink_converter.h"

#include <algorithm>
#include <string>
#include <vector>

#include "atom/common/keyboard_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "native_mate/dictionary.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"
#include "third_party/blink/public/platform/web_mouse_wheel_event.h"
#include "third_party/blink/public/web/web_device_emulation_params.h"
#include "third_party/blink/public/web/web_find_options.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace {

template <typename T>
int VectorToBitArray(const std::vector<T>& vec) {
  int bits = 0;
  for (const T& item : vec)
    bits |= item;
  return bits;
}

}  // namespace

namespace mate {

template <>
struct Converter<base::char16> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::char16* out) {
    base::string16 code = base::UTF8ToUTF16(V8ToString(val));
    if (code.length() != 1)
      return false;
    *out = code[0];
    return true;
  }
};

template <>
struct Converter<blink::WebInputEvent::Type> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebInputEvent::Type* out) {
    std::string type = base::ToLowerASCII(V8ToString(val));
    if (type == "mousedown")
      *out = blink::WebInputEvent::kMouseDown;
    else if (type == "mouseup")
      *out = blink::WebInputEvent::kMouseUp;
    else if (type == "mousemove")
      *out = blink::WebInputEvent::kMouseMove;
    else if (type == "mouseenter")
      *out = blink::WebInputEvent::kMouseEnter;
    else if (type == "mouseleave")
      *out = blink::WebInputEvent::kMouseLeave;
    else if (type == "contextmenu")
      *out = blink::WebInputEvent::kContextMenu;
    else if (type == "mousewheel")
      *out = blink::WebInputEvent::kMouseWheel;
    else if (type == "keydown")
      *out = blink::WebInputEvent::kRawKeyDown;
    else if (type == "keyup")
      *out = blink::WebInputEvent::kKeyUp;
    else if (type == "char")
      *out = blink::WebInputEvent::kChar;
    else if (type == "touchstart")
      *out = blink::WebInputEvent::kTouchStart;
    else if (type == "touchmove")
      *out = blink::WebInputEvent::kTouchMove;
    else if (type == "touchend")
      *out = blink::WebInputEvent::kTouchEnd;
    else if (type == "touchcancel")
      *out = blink::WebInputEvent::kTouchCancel;
    return true;
  }
};

template <>
struct Converter<blink::WebMouseEvent::Button> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebMouseEvent::Button* out) {
    std::string button = base::ToLowerASCII(V8ToString(val));
    if (button == "left")
      *out = blink::WebMouseEvent::Button::kLeft;
    else if (button == "middle")
      *out = blink::WebMouseEvent::Button::kMiddle;
    else if (button == "right")
      *out = blink::WebMouseEvent::Button::kRight;
    else
      return false;
    return true;
  }
};

template <>
struct Converter<blink::WebInputEvent::Modifiers> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebInputEvent::Modifiers* out) {
    std::string modifier = base::ToLowerASCII(V8ToString(val));
    if (modifier == "shift")
      *out = blink::WebInputEvent::kShiftKey;
    else if (modifier == "control" || modifier == "ctrl")
      *out = blink::WebInputEvent::kControlKey;
    else if (modifier == "alt")
      *out = blink::WebInputEvent::kAltKey;
    else if (modifier == "meta" || modifier == "command" || modifier == "cmd")
      *out = blink::WebInputEvent::kMetaKey;
    else if (modifier == "iskeypad")
      *out = blink::WebInputEvent::kIsKeyPad;
    else if (modifier == "isautorepeat")
      *out = blink::WebInputEvent::kIsAutoRepeat;
    else if (modifier == "leftbuttondown")
      *out = blink::WebInputEvent::kLeftButtonDown;
    else if (modifier == "middlebuttondown")
      *out = blink::WebInputEvent::kMiddleButtonDown;
    else if (modifier == "rightbuttondown")
      *out = blink::WebInputEvent::kRightButtonDown;
    else if (modifier == "capslock")
      *out = blink::WebInputEvent::kCapsLockOn;
    else if (modifier == "numlock")
      *out = blink::WebInputEvent::kNumLockOn;
    else if (modifier == "left")
      *out = blink::WebInputEvent::kIsLeft;
    else if (modifier == "right")
      *out = blink::WebInputEvent::kIsRight;
    return true;
  }
};

blink::WebInputEvent::Type GetWebInputEventType(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val) {
  blink::WebInputEvent::Type type = blink::WebInputEvent::kUndefined;
  mate::Dictionary dict;
  ConvertFromV8(isolate, val, &dict) && dict.Get("type", &type);
  return type;
}

bool Converter<blink::WebInputEvent>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             blink::WebInputEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  blink::WebInputEvent::Type type;
  if (!dict.Get("type", &type))
    return false;
  out->SetType(type);
  std::vector<blink::WebInputEvent::Modifiers> modifiers;
  if (dict.Get("modifiers", &modifiers))
    out->SetModifiers(VectorToBitArray(modifiers));
  out->SetTimeStamp(base::TimeTicks::Now());
  return true;
}

bool Converter<blink::WebKeyboardEvent>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                blink::WebKeyboardEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebInputEvent*>(out)))
    return false;

  std::string str;
  if (!dict.Get("keyCode", &str))
    return false;

  bool shifted = false;
  ui::KeyboardCode keyCode = atom::KeyboardCodeFromStr(str, &shifted);
  out->windows_key_code = keyCode;
  if (shifted)
    out->SetModifiers(out->GetModifiers() | blink::WebInputEvent::kShiftKey);

  ui::DomCode domCode = ui::UsLayoutKeyboardCodeToDomCode(keyCode);
  out->dom_code = static_cast<int>(domCode);

  ui::DomKey domKey;
  ui::KeyboardCode dummy_code;
  int flags = atom::WebEventModifiersToEventFlags(out->GetModifiers());
  if (ui::DomCodeToUsLayoutDomKey(domCode, flags, &domKey, &dummy_code))
    out->dom_key = static_cast<int>(domKey);

  if ((out->GetType() == blink::WebInputEvent::kChar ||
       out->GetType() == blink::WebInputEvent::kRawKeyDown)) {
    // Make sure to not read beyond the buffer in case some bad code doesn't
    // NULL-terminate it (this is called from plugins).
    size_t text_length_cap = blink::WebKeyboardEvent::kTextLengthCap;
    base::string16 text16 = base::UTF8ToUTF16(str);

    memset(out->text, 0, text_length_cap);
    memset(out->unmodified_text, 0, text_length_cap);
    for (size_t i = 0; i < std::min(text_length_cap, text16.size()); ++i) {
      out->text[i] = text16[i];
      out->unmodified_text[i] = text16[i];
    }
  }
  return true;
}

bool Converter<content::NativeWebKeyboardEvent>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    content::NativeWebKeyboardEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebKeyboardEvent*>(out)))
    return false;
  dict.Get("skipInBrowser", &out->skip_in_browser);
  return true;
}

v8::Local<v8::Value> Converter<content::NativeWebKeyboardEvent>::ToV8(
    v8::Isolate* isolate,
    const content::NativeWebKeyboardEvent& in) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);

  if (in.GetType() == blink::WebInputEvent::Type::kRawKeyDown)
    dict.Set("type", "keyDown");
  else if (in.GetType() == blink::WebInputEvent::Type::kKeyUp)
    dict.Set("type", "keyUp");
  dict.Set("key", ui::KeycodeConverter::DomKeyToKeyString(in.dom_key));
  dict.Set("code", ui::KeycodeConverter::DomCodeToCodeString(
                       static_cast<ui::DomCode>(in.dom_code)));

  using Modifiers = blink::WebInputEvent::Modifiers;
  dict.Set("isAutoRepeat", (in.GetModifiers() & Modifiers::kIsAutoRepeat) != 0);
  dict.Set("shift", (in.GetModifiers() & Modifiers::kShiftKey) != 0);
  dict.Set("control", (in.GetModifiers() & Modifiers::kControlKey) != 0);
  dict.Set("alt", (in.GetModifiers() & Modifiers::kAltKey) != 0);
  dict.Set("meta", (in.GetModifiers() & Modifiers::kMetaKey) != 0);

  return dict.GetHandle();
}

bool Converter<blink::WebMouseEvent>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             blink::WebMouseEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebInputEvent*>(out)))
    return false;

  float x = 0.f;
  float y = 0.f;
  if (!dict.Get("x", &x) || !dict.Get("y", &y))
    return false;
  out->SetPositionInWidget(x, y);

  if (!dict.Get("button", &out->button))
    out->button = blink::WebMouseEvent::Button::kLeft;

  float global_x = 0.f;
  float global_y = 0.f;
  dict.Get("globalX", &global_x);
  dict.Get("globalY", &global_y);
  out->SetPositionInScreen(global_x, global_y);

  dict.Get("movementX", &out->movement_x);
  dict.Get("movementY", &out->movement_y);
  dict.Get("clickCount", &out->click_count);
  return true;
}

bool Converter<blink::WebMouseWheelEvent>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::WebMouseWheelEvent* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebMouseEvent*>(out)))
    return false;
  dict.Get("deltaX", &out->delta_x);
  dict.Get("deltaY", &out->delta_y);
  dict.Get("wheelTicksX", &out->wheel_ticks_x);
  dict.Get("wheelTicksY", &out->wheel_ticks_y);
  dict.Get("accelerationRatioX", &out->acceleration_ratio_x);
  dict.Get("accelerationRatioY", &out->acceleration_ratio_y);
  dict.Get("hasPreciseScrollingDeltas", &out->has_precise_scrolling_deltas);

#if defined(USE_AURA)
  // Matches the behavior of ui/events/blink/web_input_event_traits.cc:
  bool can_scroll = true;
  if (dict.Get("canScroll", &can_scroll) && !can_scroll) {
    out->has_precise_scrolling_deltas = false;
    out->SetModifiers(out->GetModifiers() & ~blink::WebInputEvent::kControlKey);
  }
#endif
  return true;
}

bool Converter<blink::WebFloatPoint>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             blink::WebFloatPoint* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("x", &out->x) && dict.Get("y", &out->y);
}

template <>
struct Converter<base::Optional<blink::WebPoint>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Optional<blink::WebPoint>* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    blink::WebPoint point;
    bool success = dict.Get("x", &point.x) && dict.Get("y", &point.y);
    if (!success)
      return false;
    out->emplace(point);
    return true;
  }
};

bool Converter<blink::WebSize>::FromV8(v8::Isolate* isolate,
                                       v8::Local<v8::Value> val,
                                       blink::WebSize* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("width", &out->width) && dict.Get("height", &out->height);
}

bool Converter<blink::WebDeviceEmulationParams>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::WebDeviceEmulationParams* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  std::string screen_position;
  if (dict.Get("screenPosition", &screen_position)) {
    screen_position = base::ToLowerASCII(screen_position);
    if (screen_position == "mobile")
      out->screen_position = blink::WebDeviceEmulationParams::kMobile;
    else if (screen_position == "desktop")
      out->screen_position = blink::WebDeviceEmulationParams::kDesktop;
    else
      return false;
  }

  dict.Get("screenSize", &out->screen_size);
  dict.Get("viewPosition", &out->view_position);
  dict.Get("deviceScaleFactor", &out->device_scale_factor);
  dict.Get("viewSize", &out->view_size);
  dict.Get("scale", &out->scale);
  return true;
}

bool Converter<blink::WebFindOptions>::FromV8(v8::Isolate* isolate,
                                              v8::Local<v8::Value> val,
                                              blink::WebFindOptions* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  dict.Get("forward", &out->forward);
  dict.Get("matchCase", &out->match_case);
  dict.Get("findNext", &out->find_next);
  dict.Get("wordStart", &out->word_start);
  dict.Get("medialCapitalAsWordStart", &out->medial_capital_as_word_start);
  return true;
}

// static
v8::Local<v8::Value> Converter<blink::WebContextMenuData::MediaType>::ToV8(
    v8::Isolate* isolate,
    const blink::WebContextMenuData::MediaType& in) {
  switch (in) {
    case blink::WebContextMenuData::kMediaTypeImage:
      return mate::StringToV8(isolate, "image");
    case blink::WebContextMenuData::kMediaTypeVideo:
      return mate::StringToV8(isolate, "video");
    case blink::WebContextMenuData::kMediaTypeAudio:
      return mate::StringToV8(isolate, "audio");
    case blink::WebContextMenuData::kMediaTypeCanvas:
      return mate::StringToV8(isolate, "canvas");
    case blink::WebContextMenuData::kMediaTypeFile:
      return mate::StringToV8(isolate, "file");
    case blink::WebContextMenuData::kMediaTypePlugin:
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
    case blink::WebContextMenuData::kInputFieldTypePlainText:
      return mate::StringToV8(isolate, "plainText");
    case blink::WebContextMenuData::kInputFieldTypePassword:
      return mate::StringToV8(isolate, "password");
    case blink::WebContextMenuData::kInputFieldTypeOther:
      return mate::StringToV8(isolate, "other");
    default:
      return mate::StringToV8(isolate, "none");
  }
}

v8::Local<v8::Value> EditFlagsToV8(v8::Isolate* isolate, int editFlags) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("canUndo", !!(editFlags & blink::WebContextMenuData::kCanUndo));
  dict.Set("canRedo", !!(editFlags & blink::WebContextMenuData::kCanRedo));
  dict.Set("canCut", !!(editFlags & blink::WebContextMenuData::kCanCut));
  dict.Set("canCopy", !!(editFlags & blink::WebContextMenuData::kCanCopy));

  bool pasteFlag = false;
  if (editFlags & blink::WebContextMenuData::kCanPaste) {
    std::vector<base::string16> types;
    bool ignore;
    ui::Clipboard::GetForCurrentThread()->ReadAvailableTypes(
        ui::CLIPBOARD_TYPE_COPY_PASTE, &types, &ignore);
    pasteFlag = !types.empty();
  }
  dict.Set("canPaste", pasteFlag);

  dict.Set("canDelete", !!(editFlags & blink::WebContextMenuData::kCanDelete));
  dict.Set("canSelectAll",
           !!(editFlags & blink::WebContextMenuData::kCanSelectAll));

  return mate::ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("inError",
           !!(mediaFlags & blink::WebContextMenuData::kMediaInError));
  dict.Set("isPaused",
           !!(mediaFlags & blink::WebContextMenuData::kMediaPaused));
  dict.Set("isMuted", !!(mediaFlags & blink::WebContextMenuData::kMediaMuted));
  dict.Set("hasAudio",
           !!(mediaFlags & blink::WebContextMenuData::kMediaHasAudio));
  dict.Set("isLooping",
           (mediaFlags & blink::WebContextMenuData::kMediaLoop) != 0);
  dict.Set("isControlsVisible",
           (mediaFlags & blink::WebContextMenuData::kMediaControls) != 0);
  dict.Set("canToggleControls",
           !!(mediaFlags & blink::WebContextMenuData::kMediaCanToggleControls));
  dict.Set("canRotate",
           !!(mediaFlags & blink::WebContextMenuData::kMediaCanRotate));
  return mate::ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> Converter<blink::WebCache::ResourceTypeStat>::ToV8(
    v8::Isolate* isolate,
    const blink::WebCache::ResourceTypeStat& stat) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("count", static_cast<uint32_t>(stat.count));
  dict.Set("size", static_cast<double>(stat.size));
  dict.Set("liveSize", static_cast<double>(stat.decoded_size));
  return dict.GetHandle();
}

v8::Local<v8::Value> Converter<blink::WebCache::ResourceTypeStats>::ToV8(
    v8::Isolate* isolate,
    const blink::WebCache::ResourceTypeStats& stats) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("images", stats.images);
  dict.Set("scripts", stats.scripts);
  dict.Set("cssStyleSheets", stats.css_style_sheets);
  dict.Set("xslStyleSheets", stats.xsl_style_sheets);
  dict.Set("fonts", stats.fonts);
  dict.Set("other", stats.other);
  return dict.GetHandle();
}

// static
v8::Local<v8::Value> Converter<blink::WebReferrerPolicy>::ToV8(
    v8::Isolate* isolate,
    const blink::WebReferrerPolicy& in) {
  switch (in) {
    case blink::kWebReferrerPolicyDefault:
      return mate::StringToV8(isolate, "default");
    case blink::kWebReferrerPolicyAlways:
      return mate::StringToV8(isolate, "unsafe-url");
    case blink::kWebReferrerPolicyNoReferrerWhenDowngrade:
      return mate::StringToV8(isolate, "no-referrer-when-downgrade");
    case blink::kWebReferrerPolicyNever:
      return mate::StringToV8(isolate, "no-referrer");
    case blink::kWebReferrerPolicyOrigin:
      return mate::StringToV8(isolate, "origin");
    case blink::kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin:
      return mate::StringToV8(isolate, "strict-origin-when-cross-origin");
    case blink::kWebReferrerPolicySameOrigin:
      return mate::StringToV8(isolate, "same-origin");
    case blink::kWebReferrerPolicyStrictOrigin:
      return mate::StringToV8(isolate, "strict-origin");
    default:
      return mate::StringToV8(isolate, "no-referrer");
  }
}

// static
bool Converter<blink::WebReferrerPolicy>::FromV8(
    v8::Isolate* isolate,
    v8::Handle<v8::Value> val,
    blink::WebReferrerPolicy* out) {
  std::string policy = base::ToLowerASCII(V8ToString(val));
  if (policy == "default")
    *out = blink::kWebReferrerPolicyDefault;
  else if (policy == "unsafe-url")
    *out = blink::kWebReferrerPolicyAlways;
  else if (policy == "no-referrer-when-downgrade")
    *out = blink::kWebReferrerPolicyNoReferrerWhenDowngrade;
  else if (policy == "no-referrer")
    *out = blink::kWebReferrerPolicyNever;
  else if (policy == "origin")
    *out = blink::kWebReferrerPolicyOrigin;
  else if (policy == "strict-origin-when-cross-origin")
    *out =
        blink::kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin;
  else if (policy == "same-origin")
    *out = blink::kWebReferrerPolicySameOrigin;
  else if (policy == "strict-origin")
    *out = blink::kWebReferrerPolicyStrictOrigin;
  else
    return false;
  return true;
}

}  // namespace mate
