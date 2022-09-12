// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/blink_converter.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/keyboard_util.h"
#include "shell/common/v8_value_serializer.h"
#include "third_party/blink/public/common/context_menu_data/edit_flags.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_keyboard_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "third_party/blink/public/common/widget/device_emulation_params.h"
#include "third_party/blink/public/mojom/loader/referrer.mojom.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/events/blink/blink_event_util.h"
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

namespace gin {

template <>
struct Converter<char16_t> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     char16_t* out) {
    std::u16string code = base::UTF8ToUTF16(gin::V8ToString(isolate, val));
    if (code.length() != 1)
      return false;
    *out = code[0];
    return true;
  }
};

#define BLINK_EVENT_TYPES()                                  \
  CASE_TYPE(kUndefined, "undefined")                         \
  CASE_TYPE(kMouseDown, "mouseDown")                         \
  CASE_TYPE(kMouseUp, "mouseUp")                             \
  CASE_TYPE(kMouseMove, "mouseMove")                         \
  CASE_TYPE(kMouseEnter, "mouseEnter")                       \
  CASE_TYPE(kMouseLeave, "mouseLeave")                       \
  CASE_TYPE(kContextMenu, "contextMenu")                     \
  CASE_TYPE(kMouseWheel, "mouseWheel")                       \
  CASE_TYPE(kRawKeyDown, "rawKeyDown")                       \
  CASE_TYPE(kKeyDown, "keyDown")                             \
  CASE_TYPE(kKeyUp, "keyUp")                                 \
  CASE_TYPE(kChar, "char")                                   \
  CASE_TYPE(kGestureScrollBegin, "gestureScrollBegin")       \
  CASE_TYPE(kGestureScrollEnd, "gestureScrollEnd")           \
  CASE_TYPE(kGestureScrollUpdate, "gestureScrollUpdate")     \
  CASE_TYPE(kGestureFlingStart, "gestureFlingStart")         \
  CASE_TYPE(kGestureFlingCancel, "gestureFlingCancel")       \
  CASE_TYPE(kGesturePinchBegin, "gesturePinchBegin")         \
  CASE_TYPE(kGesturePinchEnd, "gesturePinchEnd")             \
  CASE_TYPE(kGesturePinchUpdate, "gesturePinchUpdate")       \
  CASE_TYPE(kGestureTapDown, "gestureTapDown")               \
  CASE_TYPE(kGestureShowPress, "gestureShowPress")           \
  CASE_TYPE(kGestureTap, "gestureTap")                       \
  CASE_TYPE(kGestureTapCancel, "gestureTapCancel")           \
  CASE_TYPE(kGestureShortPress, "gestureShortPress")         \
  CASE_TYPE(kGestureLongPress, "gestureLongPress")           \
  CASE_TYPE(kGestureLongTap, "gestureLongTap")               \
  CASE_TYPE(kGestureTwoFingerTap, "gestureTwoFingerTap")     \
  CASE_TYPE(kGestureTapUnconfirmed, "gestureTapUnconfirmed") \
  CASE_TYPE(kGestureDoubleTap, "gestureDoubleTap")           \
  CASE_TYPE(kTouchStart, "touchStart")                       \
  CASE_TYPE(kTouchMove, "touchMove")                         \
  CASE_TYPE(kTouchEnd, "touchEnd")                           \
  CASE_TYPE(kTouchCancel, "touchCancel")                     \
  CASE_TYPE(kTouchScrollStarted, "touchScrollStarted")       \
  CASE_TYPE(kPointerDown, "pointerDown")                     \
  CASE_TYPE(kPointerUp, "pointerUp")                         \
  CASE_TYPE(kPointerMove, "pointerMove")                     \
  CASE_TYPE(kPointerRawUpdate, "pointerRawUpdate")           \
  CASE_TYPE(kPointerCancel, "pointerCancel")                 \
  CASE_TYPE(kPointerCausedUaAction, "pointerCausedUaAction")

bool Converter<blink::WebInputEvent::Type>::FromV8(
    v8::Isolate* isolate,
    v8::Handle<v8::Value> val,
    blink::WebInputEvent::Type* out) {
  std::string type = gin::V8ToString(isolate, val);
#define CASE_TYPE(event_type, js_name)                   \
  if (base::EqualsCaseInsensitiveASCII(type, js_name)) { \
    *out = blink::WebInputEvent::Type::event_type;       \
    return true;                                         \
  }
  BLINK_EVENT_TYPES()
#undef CASE_TYPE
  return false;
}

v8::Local<v8::Value> Converter<blink::WebInputEvent::Type>::ToV8(
    v8::Isolate* isolate,
    const blink::WebInputEvent::Type& in) {
#define CASE_TYPE(event_type, js_name)         \
  case blink::WebInputEvent::Type::event_type: \
    return StringToV8(isolate, js_name);
  switch (in) { BLINK_EVENT_TYPES() }
#undef CASE_TYPE
}

template <>
struct Converter<blink::WebMouseEvent::Button> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebMouseEvent::Button* out) {
    std::string button = base::ToLowerASCII(gin::V8ToString(isolate, val));
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
    std::string modifier = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (modifier == "shift")
      *out = blink::WebInputEvent::Modifiers::kShiftKey;
    else if (modifier == "control" || modifier == "ctrl")
      *out = blink::WebInputEvent::Modifiers::kControlKey;
    else if (modifier == "alt")
      *out = blink::WebInputEvent::Modifiers::kAltKey;
    else if (modifier == "meta" || modifier == "command" || modifier == "cmd")
      *out = blink::WebInputEvent::Modifiers::kMetaKey;
    else if (modifier == "iskeypad")
      *out = blink::WebInputEvent::Modifiers::kIsKeyPad;
    else if (modifier == "isautorepeat")
      *out = blink::WebInputEvent::Modifiers::kIsAutoRepeat;
    else if (modifier == "leftbuttondown")
      *out = blink::WebInputEvent::Modifiers::kLeftButtonDown;
    else if (modifier == "middlebuttondown")
      *out = blink::WebInputEvent::Modifiers::kMiddleButtonDown;
    else if (modifier == "rightbuttondown")
      *out = blink::WebInputEvent::Modifiers::kRightButtonDown;
    else if (modifier == "capslock")
      *out = blink::WebInputEvent::Modifiers::kCapsLockOn;
    else if (modifier == "numlock")
      *out = blink::WebInputEvent::Modifiers::kNumLockOn;
    else if (modifier == "left")
      *out = blink::WebInputEvent::Modifiers::kIsLeft;
    else if (modifier == "right")
      *out = blink::WebInputEvent::Modifiers::kIsRight;
    // TODO(nornagon): the rest of the modifiers
    return true;
  }
};

std::vector<std::string> ModifiersToArray(int modifiers) {
  using Modifiers = blink::WebInputEvent::Modifiers;
  std::vector<std::string> modifier_strings;
  if (modifiers & Modifiers::kShiftKey)
    modifier_strings.push_back("shift");
  if (modifiers & Modifiers::kControlKey)
    modifier_strings.push_back("control");
  if (modifiers & Modifiers::kAltKey)
    modifier_strings.push_back("alt");
  if (modifiers & Modifiers::kMetaKey)
    modifier_strings.push_back("meta");
  if (modifiers & Modifiers::kIsKeyPad)
    modifier_strings.push_back("iskeypad");
  if (modifiers & Modifiers::kIsAutoRepeat)
    modifier_strings.push_back("isautorepeat");
  if (modifiers & Modifiers::kLeftButtonDown)
    modifier_strings.push_back("leftbuttondown");
  if (modifiers & Modifiers::kMiddleButtonDown)
    modifier_strings.push_back("middlebuttondown");
  if (modifiers & Modifiers::kRightButtonDown)
    modifier_strings.push_back("rightbuttondown");
  if (modifiers & Modifiers::kCapsLockOn)
    modifier_strings.push_back("capslock");
  if (modifiers & Modifiers::kNumLockOn)
    modifier_strings.push_back("numlock");
  if (modifiers & Modifiers::kIsLeft)
    modifier_strings.push_back("left");
  if (modifiers & Modifiers::kIsRight)
    modifier_strings.push_back("right");
  // TODO(nornagon): the rest of the modifiers
  return modifier_strings;
}

blink::WebInputEvent::Type GetWebInputEventType(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val) {
  blink::WebInputEvent::Type type = blink::WebInputEvent::Type::kUndefined;
  gin_helper::Dictionary dict;
  ConvertFromV8(isolate, val, &dict) && dict.Get("type", &type);
  return type;
}

bool Converter<blink::WebInputEvent>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             blink::WebInputEvent* out) {
  gin_helper::Dictionary dict;
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

v8::Local<v8::Value> Converter<blink::WebInputEvent>::ToV8(
    v8::Isolate* isolate,
    const blink::WebInputEvent& in) {
  if (blink::WebInputEvent::IsKeyboardEventType(in.GetType()))
    return gin::ConvertToV8(isolate,
                            *static_cast<const blink::WebKeyboardEvent*>(&in));
  return gin::DataObjectBuilder(isolate)
      .Set("type", in.GetType())
      .Set("modifiers", ModifiersToArray(in.GetModifiers()))
      .Set("_modifiers", in.GetModifiers())
      .Build();
}

bool Converter<blink::WebKeyboardEvent>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                blink::WebKeyboardEvent* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebInputEvent*>(out)))
    return false;

  std::string str;
  if (!dict.Get("keyCode", &str))
    return false;

  absl::optional<char16_t> shifted_char;
  ui::KeyboardCode keyCode = electron::KeyboardCodeFromStr(str, &shifted_char);
  out->windows_key_code = keyCode;
  if (shifted_char)
    out->SetModifiers(out->GetModifiers() |
                      blink::WebInputEvent::Modifiers::kShiftKey);

  ui::DomCode domCode = ui::UsLayoutKeyboardCodeToDomCode(keyCode);
  out->dom_code = static_cast<int>(domCode);

  ui::DomKey domKey;
  ui::KeyboardCode dummy_code;
  int flags = ui::WebEventModifiersToEventFlags(out->GetModifiers());
  if (ui::DomCodeToUsLayoutDomKey(domCode, flags, &domKey, &dummy_code))
    out->dom_key = static_cast<int>(domKey);

  if ((out->GetType() == blink::WebInputEvent::Type::kChar ||
       out->GetType() == blink::WebInputEvent::Type::kRawKeyDown)) {
    // Make sure to not read beyond the buffer in case some bad code doesn't
    // NULL-terminate it (this is called from plugins).
    size_t text_length_cap = blink::WebKeyboardEvent::kTextLengthCap;
    std::u16string text16 = base::UTF8ToUTF16(str);

    std::fill_n(out->text, text_length_cap, 0);
    std::fill_n(out->unmodified_text, text_length_cap, 0);
    for (size_t i = 0; i < std::min(text_length_cap - 1, text16.size()); ++i) {
      out->text[i] = text16[i];
      out->unmodified_text[i] = text16[i];
    }
  }
  return true;
}

int GetKeyLocationCode(const blink::WebInputEvent& key) {
  // https://source.chromium.org/chromium/chromium/src/+/main:third_party/blink/renderer/core/events/keyboard_event.h;l=46;drc=1ff6437e65b183e673b7b4f25060b74dc2ba5c37
  enum KeyLocationCode {
    kDomKeyLocationStandard = 0x00,
    kDomKeyLocationLeft = 0x01,
    kDomKeyLocationRight = 0x02,
    kDomKeyLocationNumpad = 0x03
  };
  using Modifiers = blink::WebInputEvent::Modifiers;
  if (key.GetModifiers() & Modifiers::kIsKeyPad)
    return kDomKeyLocationNumpad;
  if (key.GetModifiers() & Modifiers::kIsLeft)
    return kDomKeyLocationLeft;
  if (key.GetModifiers() & Modifiers::kIsRight)
    return kDomKeyLocationRight;
  return kDomKeyLocationStandard;
}

v8::Local<v8::Value> Converter<blink::WebKeyboardEvent>::ToV8(
    v8::Isolate* isolate,
    const blink::WebKeyboardEvent& in) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("type", in.GetType());
  dict.Set("key", ui::KeycodeConverter::DomKeyToKeyString(in.dom_key));
  dict.Set("code", ui::KeycodeConverter::DomCodeToCodeString(
                       static_cast<ui::DomCode>(in.dom_code)));

  using Modifiers = blink::WebInputEvent::Modifiers;
  dict.Set("isAutoRepeat", (in.GetModifiers() & Modifiers::kIsAutoRepeat) != 0);
  dict.Set("isComposing", (in.GetModifiers() & Modifiers::kIsComposing) != 0);
  dict.Set("shift", (in.GetModifiers() & Modifiers::kShiftKey) != 0);
  dict.Set("control", (in.GetModifiers() & Modifiers::kControlKey) != 0);
  dict.Set("alt", (in.GetModifiers() & Modifiers::kAltKey) != 0);
  dict.Set("meta", (in.GetModifiers() & Modifiers::kMetaKey) != 0);
  dict.Set("location", GetKeyLocationCode(in));
  dict.Set("_modifiers", in.GetModifiers());
  dict.Set("modifiers", ModifiersToArray(in.GetModifiers()));

  return dict.GetHandle();
}

bool Converter<blink::WebMouseEvent>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             blink::WebMouseEvent* out) {
  gin_helper::Dictionary dict;
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
  gin_helper::Dictionary dict;
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

  bool has_precise_scrolling_deltas = false;
  dict.Get("hasPreciseScrollingDeltas", &has_precise_scrolling_deltas);
  if (has_precise_scrolling_deltas) {
    out->delta_units = ui::ScrollGranularity::kScrollByPrecisePixel;
  } else {
    out->delta_units = ui::ScrollGranularity::kScrollByPixel;
  }

#if defined(USE_AURA)
  // Matches the behavior of ui/events/blink/web_input_event_traits.cc:
  bool can_scroll = true;
  if (dict.Get("canScroll", &can_scroll) && !can_scroll) {
    out->delta_units = ui::ScrollGranularity::kScrollByPage;
    out->SetModifiers(out->GetModifiers() &
                      ~blink::WebInputEvent::Modifiers::kControlKey);
  }
#endif
  return true;
}

bool Converter<blink::DeviceEmulationParams>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::DeviceEmulationParams* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  std::string screen_type;
  if (dict.Get("screenPosition", &screen_type)) {
    screen_type = base::ToLowerASCII(screen_type);
    if (screen_type == "mobile")
      out->screen_type = blink::mojom::EmulatedScreenType::kMobile;
    else if (screen_type == "desktop")
      out->screen_type = blink::mojom::EmulatedScreenType::kDesktop;
    else
      return false;
  }

  dict.Get("screenSize", &out->screen_size);
  gfx::Point view_position;
  if (dict.Get("viewPosition", &view_position)) {
    out->view_position = view_position;
  }
  dict.Get("deviceScaleFactor", &out->device_scale_factor);
  dict.Get("viewSize", &out->view_size);
  dict.Get("scale", &out->scale);
  return true;
}

// static
v8::Local<v8::Value> Converter<blink::mojom::ContextMenuDataMediaType>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::ContextMenuDataMediaType& in) {
  switch (in) {
    case blink::mojom::ContextMenuDataMediaType::kImage:
      return StringToV8(isolate, "image");
    case blink::mojom::ContextMenuDataMediaType::kVideo:
      return StringToV8(isolate, "video");
    case blink::mojom::ContextMenuDataMediaType::kAudio:
      return StringToV8(isolate, "audio");
    case blink::mojom::ContextMenuDataMediaType::kCanvas:
      return StringToV8(isolate, "canvas");
    case blink::mojom::ContextMenuDataMediaType::kFile:
      return StringToV8(isolate, "file");
    case blink::mojom::ContextMenuDataMediaType::kPlugin:
      return StringToV8(isolate, "plugin");
    default:
      return StringToV8(isolate, "none");
  }
}

// static
v8::Local<v8::Value>
Converter<blink::mojom::ContextMenuDataInputFieldType>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::ContextMenuDataInputFieldType& in) {
  switch (in) {
    case blink::mojom::ContextMenuDataInputFieldType::kPlainText:
      return StringToV8(isolate, "plainText");
    case blink::mojom::ContextMenuDataInputFieldType::kPassword:
      return StringToV8(isolate, "password");
    case blink::mojom::ContextMenuDataInputFieldType::kOther:
      return StringToV8(isolate, "other");
    default:
      return StringToV8(isolate, "none");
  }
}

v8::Local<v8::Value> EditFlagsToV8(v8::Isolate* isolate, int editFlags) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("canUndo",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanUndo));
  dict.Set("canRedo",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanRedo));
  dict.Set("canCut", !!(editFlags & blink::ContextMenuDataEditFlags::kCanCut));
  dict.Set("canCopy",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanCopy));

  bool pasteFlag = false;
  if (editFlags & blink::ContextMenuDataEditFlags::kCanPaste) {
    std::vector<std::u16string> types;
    ui::Clipboard::GetForCurrentThread()->ReadAvailableTypes(
        ui::ClipboardBuffer::kCopyPaste, /* data_dst = */ nullptr, &types);
    pasteFlag = !types.empty();
  }
  dict.Set("canPaste", pasteFlag);

  dict.Set("canDelete",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanDelete));
  dict.Set("canSelectAll",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanSelectAll));
  dict.Set("canEditRichly",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanEditRichly));

  return ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("inError", !!(mediaFlags & blink::ContextMenuData::kMediaInError));
  dict.Set("isPaused", !!(mediaFlags & blink::ContextMenuData::kMediaPaused));
  dict.Set("isMuted", !!(mediaFlags & blink::ContextMenuData::kMediaMuted));
  dict.Set("canSave", !!(mediaFlags & blink::ContextMenuData::kMediaCanSave));
  dict.Set("hasAudio", !!(mediaFlags & blink::ContextMenuData::kMediaHasAudio));
  dict.Set("isLooping", !!(mediaFlags & blink::ContextMenuData::kMediaLoop));
  dict.Set("isControlsVisible",
           !!(mediaFlags & blink::ContextMenuData::kMediaControls));
  dict.Set("canToggleControls",
           !!(mediaFlags & blink::ContextMenuData::kMediaCanToggleControls));
  dict.Set("canPrint", !!(mediaFlags & blink::ContextMenuData::kMediaCanPrint));
  dict.Set("canRotate",
           !!(mediaFlags & blink::ContextMenuData::kMediaCanRotate));
  dict.Set("canShowPictureInPicture",
           !!(mediaFlags & blink::ContextMenuData::kMediaCanPictureInPicture));
  dict.Set("isShowingPictureInPicture",
           !!(mediaFlags & blink::ContextMenuData::kMediaPictureInPicture));
  dict.Set("canLoop", !!(mediaFlags & blink::ContextMenuData::kMediaCanLoop));
  return ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> Converter<blink::WebCacheResourceTypeStat>::ToV8(
    v8::Isolate* isolate,
    const blink::WebCacheResourceTypeStat& stat) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("count", static_cast<uint32_t>(stat.count));
  dict.Set("size", static_cast<double>(stat.size));
  dict.Set("liveSize", static_cast<double>(stat.decoded_size));
  return dict.GetHandle();
}

v8::Local<v8::Value> Converter<blink::WebCacheResourceTypeStats>::ToV8(
    v8::Isolate* isolate,
    const blink::WebCacheResourceTypeStats& stats) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("images", stats.images);
  dict.Set("scripts", stats.scripts);
  dict.Set("cssStyleSheets", stats.css_style_sheets);
  dict.Set("xslStyleSheets", stats.xsl_style_sheets);
  dict.Set("fonts", stats.fonts);
  dict.Set("other", stats.other);
  return dict.GetHandle();
}

// static
v8::Local<v8::Value> Converter<network::mojom::ReferrerPolicy>::ToV8(
    v8::Isolate* isolate,
    const network::mojom::ReferrerPolicy& in) {
  switch (in) {
    case network::mojom::ReferrerPolicy::kDefault:
      return StringToV8(isolate, "default");
    case network::mojom::ReferrerPolicy::kAlways:
      return StringToV8(isolate, "unsafe-url");
    case network::mojom::ReferrerPolicy::kNoReferrerWhenDowngrade:
      return StringToV8(isolate, "no-referrer-when-downgrade");
    case network::mojom::ReferrerPolicy::kNever:
      return StringToV8(isolate, "no-referrer");
    case network::mojom::ReferrerPolicy::kOrigin:
      return StringToV8(isolate, "origin");
    case network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin:
      return StringToV8(isolate, "strict-origin-when-cross-origin");
    case network::mojom::ReferrerPolicy::kSameOrigin:
      return StringToV8(isolate, "same-origin");
    case network::mojom::ReferrerPolicy::kStrictOrigin:
      return StringToV8(isolate, "strict-origin");
    default:
      return StringToV8(isolate, "no-referrer");
  }
}

// static
bool Converter<network::mojom::ReferrerPolicy>::FromV8(
    v8::Isolate* isolate,
    v8::Handle<v8::Value> val,
    network::mojom::ReferrerPolicy* out) {
  std::string policy = base::ToLowerASCII(gin::V8ToString(isolate, val));
  if (policy == "default")
    *out = network::mojom::ReferrerPolicy::kDefault;
  else if (policy == "unsafe-url")
    *out = network::mojom::ReferrerPolicy::kAlways;
  else if (policy == "no-referrer-when-downgrade")
    *out = network::mojom::ReferrerPolicy::kNoReferrerWhenDowngrade;
  else if (policy == "no-referrer")
    *out = network::mojom::ReferrerPolicy::kNever;
  else if (policy == "origin")
    *out = network::mojom::ReferrerPolicy::kOrigin;
  else if (policy == "strict-origin-when-cross-origin")
    *out = network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin;
  else if (policy == "same-origin")
    *out = network::mojom::ReferrerPolicy::kSameOrigin;
  else if (policy == "strict-origin")
    *out = network::mojom::ReferrerPolicy::kStrictOrigin;
  else
    return false;
  return true;
}

// static
v8::Local<v8::Value> Converter<blink::mojom::Referrer>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::Referrer& val) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("url", ConvertToV8(isolate, val.url));
  dict.Set("policy", ConvertToV8(isolate, val.policy));
  return gin::ConvertToV8(isolate, dict);
}
//
// static
bool Converter<blink::mojom::Referrer>::FromV8(v8::Isolate* isolate,
                                               v8::Local<v8::Value> val,
                                               blink::mojom::Referrer* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  if (!dict.Get("url", &out->url))
    return false;

  if (!dict.Get("policy", &out->policy))
    return false;

  return true;
}

v8::Local<v8::Value> Converter<blink::CloneableMessage>::ToV8(
    v8::Isolate* isolate,
    const blink::CloneableMessage& in) {
  return electron::DeserializeV8Value(isolate, in);
}

bool Converter<blink::CloneableMessage>::FromV8(v8::Isolate* isolate,
                                                v8::Handle<v8::Value> val,
                                                blink::CloneableMessage* out) {
  return electron::SerializeV8Value(isolate, val, out);
}

}  // namespace gin
