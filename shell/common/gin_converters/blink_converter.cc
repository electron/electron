// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/blink_converter.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/keyboard_util.h"
#include "shell/common/string_lookup.h"

#include "shell/common/v8_value_serializer.h"
#include "third_party/blink/public/common/context_menu_data/edit_flags.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/input/web_keyboard_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "third_party/blink/public/common/widget/device_emulation_params.h"
#include "third_party/blink/public/platform/web_size.h"
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

namespace gin {

template <>
struct Converter<base::char16> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::char16* out) {
    base::string16 code = base::UTF8ToUTF16(gin::V8ToString(isolate, val));
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
    static constexpr electron::StringLookup<blink::WebInputEvent::Type, 14>
        evmap{{{
            {"char", blink::WebInputEvent::Type::kChar},
            {"contextmenu", blink::WebInputEvent::Type::kContextMenu},
            {"keydown", blink::WebInputEvent::Type::kRawKeyDown},
            {"keyup", blink::WebInputEvent::Type::kKeyUp},
            {"mousedown", blink::WebInputEvent::Type::kMouseDown},
            {"mouseenter", blink::WebInputEvent::Type::kMouseEnter},
            {"mouseleave", blink::WebInputEvent::Type::kMouseLeave},
            {"mousemove", blink::WebInputEvent::Type::kMouseMove},
            {"mouseup", blink::WebInputEvent::Type::kMouseUp},
            {"mousewheel", blink::WebInputEvent::Type::kMouseWheel},
            {"touchcancel", blink::WebInputEvent::Type::kTouchCancel},
            {"touchend", blink::WebInputEvent::Type::kTouchEnd},
            {"touchmove", blink::WebInputEvent::Type::kTouchMove},
            {"touchstart", blink::WebInputEvent::Type::kTouchStart},
        }}};
    static_assert(evmap.IsSorted(), "please sort evmap");
    auto const event =
        evmap.lookup(base::ToLowerASCII(gin::V8ToString(isolate, val)));
    if (event) {
      *out = *event;
    }
    return true;
  }
};

template <>
struct Converter<blink::WebMouseEvent::Button> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebMouseEvent::Button* out) {
    static constexpr electron::StringLookup<blink::WebMouseEvent::Button, 3>
        evmap{{{{"left", blink::WebMouseEvent::Button::kLeft},
                {"middle", blink::WebMouseEvent::Button::kMiddle},
                {"right", blink::WebMouseEvent::Button::kRight}}}};
    static_assert(evmap.IsSorted(), "please sort evmap");
    auto const event =
        evmap.lookup(base::ToLowerASCII(gin::V8ToString(isolate, val)));
    if (event) {
      *out = *event;
      return true;
    }
    return false;
  }
};

template <>
struct Converter<blink::WebInputEvent::Modifiers> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     blink::WebInputEvent::Modifiers* out) {
    static constexpr electron::StringLookup<blink::WebMouseEvent::Modifiers, 16>
        modmap{{{
            {"alt", blink::WebInputEvent::Modifiers::kAltKey},
            {"capslock", blink::WebInputEvent::Modifiers::kCapsLockOn},
            {"cmd", blink::WebInputEvent::Modifiers::kMetaKey},
            {"command", blink::WebInputEvent::Modifiers::kMetaKey},
            {"control", blink::WebInputEvent::Modifiers::kControlKey},
            {"ctrl", blink::WebInputEvent::Modifiers::kControlKey},
            {"isautorepeat", blink::WebInputEvent::Modifiers::kIsAutoRepeat},
            {"iskeypad", blink::WebInputEvent::Modifiers::kIsKeyPad},
            {"left", blink::WebInputEvent::Modifiers::kIsLeft},
            {"leftbuttondown",
             blink::WebInputEvent::Modifiers::kLeftButtonDown},
            {"meta", blink::WebInputEvent::Modifiers::kMetaKey},
            {"middlebuttondown",
             blink::WebInputEvent::Modifiers::kMiddleButtonDown},
            {"numlock", blink::WebInputEvent::Modifiers::kNumLockOn},
            {"right", blink::WebInputEvent::Modifiers::kIsRight},
            {"rightbuttondown",
             blink::WebInputEvent::Modifiers::kRightButtonDown},
            {"shift", blink::WebInputEvent::Modifiers::kShiftKey},
        }}};
    static_assert(modmap.IsSorted(), "please sort mods");
    auto const mod =
        modmap.lookup(base::ToLowerASCII(gin::V8ToString(isolate, val)));
    if (mod) {
      *out = *mod;
    }
    return true;
  }
};

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

  bool shifted = false;
  ui::KeyboardCode keyCode = electron::KeyboardCodeFromStr(str, &shifted);
  out->windows_key_code = keyCode;
  if (shifted)
    out->SetModifiers(out->GetModifiers() |
                      blink::WebInputEvent::Modifiers::kShiftKey);

  ui::DomCode domCode = ui::UsLayoutKeyboardCodeToDomCode(keyCode);
  out->dom_code = static_cast<int>(domCode);

  ui::DomKey domKey;
  ui::KeyboardCode dummy_code;
  int flags = electron::WebEventModifiersToEventFlags(out->GetModifiers());
  if (ui::DomCodeToUsLayoutDomKey(domCode, flags, &domKey, &dummy_code))
    out->dom_key = static_cast<int>(domKey);

  if ((out->GetType() == blink::WebInputEvent::Type::kChar ||
       out->GetType() == blink::WebInputEvent::Type::kRawKeyDown)) {
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

bool Converter<blink::WebSize>::FromV8(v8::Isolate* isolate,
                                       v8::Local<v8::Value> val,
                                       blink::WebSize* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  return dict.Get("width", &out->width) && dict.Get("height", &out->height);
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
v8::Local<v8::Value> Converter<blink::ContextMenuDataMediaType>::ToV8(
    v8::Isolate* isolate,
    const blink::ContextMenuDataMediaType& in) {
  switch (in) {
    case blink::ContextMenuDataMediaType::kImage:
      return StringToV8(isolate, "image");
    case blink::ContextMenuDataMediaType::kVideo:
      return StringToV8(isolate, "video");
    case blink::ContextMenuDataMediaType::kAudio:
      return StringToV8(isolate, "audio");
    case blink::ContextMenuDataMediaType::kCanvas:
      return StringToV8(isolate, "canvas");
    case blink::ContextMenuDataMediaType::kFile:
      return StringToV8(isolate, "file");
    case blink::ContextMenuDataMediaType::kPlugin:
      return StringToV8(isolate, "plugin");
    default:
      return StringToV8(isolate, "none");
  }
}

// static
v8::Local<v8::Value> Converter<blink::ContextMenuDataInputFieldType>::ToV8(
    v8::Isolate* isolate,
    const blink::ContextMenuDataInputFieldType& in) {
  switch (in) {
    case blink::ContextMenuDataInputFieldType::kPlainText:
      return StringToV8(isolate, "plainText");
    case blink::ContextMenuDataInputFieldType::kPassword:
      return StringToV8(isolate, "password");
    case blink::ContextMenuDataInputFieldType::kOther:
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
    std::vector<base::string16> types;
    ui::Clipboard::GetForCurrentThread()->ReadAvailableTypes(
        ui::ClipboardBuffer::kCopyPaste, /* data_dst = */ nullptr, &types);
    pasteFlag = !types.empty();
  }
  dict.Set("canPaste", pasteFlag);

  dict.Set("canDelete",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanDelete));
  dict.Set("canSelectAll",
           !!(editFlags & blink::ContextMenuDataEditFlags::kCanSelectAll));

  return ConvertToV8(isolate, dict);
}

v8::Local<v8::Value> MediaFlagsToV8(v8::Isolate* isolate, int mediaFlags) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
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
  static constexpr electron::StringLookup<network::mojom::ReferrerPolicy, 8>
      polmap{{{
          {"default", network::mojom::ReferrerPolicy::kDefault},
          {"no-referrer", network::mojom::ReferrerPolicy::kNever},
          {"no-referrer-when-downgrade",
           network::mojom::ReferrerPolicy::kNoReferrerWhenDowngrade},
          {"origin", network::mojom::ReferrerPolicy::kOrigin},
          {"same-origin", network::mojom::ReferrerPolicy::kSameOrigin},
          {"strict-origin", network::mojom::ReferrerPolicy::kStrictOrigin},
          {"strict-origin-when-cross-origin",
           network::mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin},
          {"unsafe-url", network::mojom::ReferrerPolicy::kAlways},
      }}};
  static_assert(polmap.IsSorted(), "please sort polmap");
  auto const policy =
      polmap.lookup(base::ToLowerASCII(gin::V8ToString(isolate, val)));
  if (policy) {
    *out = *policy;
    return true;
  }
  return false;
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
