// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/content_converter.h"

#include <string>

#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/common/context_menu_data/untrustworthy_context_menu_params.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"

namespace gin {

template <>
struct Converter<ui::MenuSourceType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::MenuSourceType& in) {
    switch (in) {
      case ui::MENU_SOURCE_MOUSE:
        return StringToV8(isolate, "mouse");
      case ui::MENU_SOURCE_KEYBOARD:
        return StringToV8(isolate, "keyboard");
      case ui::MENU_SOURCE_TOUCH:
        return StringToV8(isolate, "touch");
      case ui::MENU_SOURCE_TOUCH_EDIT_MENU:
        return StringToV8(isolate, "touchMenu");
      case ui::MENU_SOURCE_LONG_PRESS:
        return StringToV8(isolate, "longPress");
      case ui::MENU_SOURCE_LONG_TAP:
        return StringToV8(isolate, "longTap");
      case ui::MENU_SOURCE_TOUCH_HANDLE:
        return StringToV8(isolate, "touchHandle");
      case ui::MENU_SOURCE_STYLUS:
        return StringToV8(isolate, "stylus");
      case ui::MENU_SOURCE_ADJUST_SELECTION:
        return StringToV8(isolate, "adjustSelection");
      case ui::MENU_SOURCE_ADJUST_SELECTION_RESET:
        return StringToV8(isolate, "adjustSelectionReset");
      default:
        return StringToV8(isolate, "none");
    }
  }
};

// static
v8::Local<v8::Value> Converter<blink::mojom::MenuItem::Type>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::MenuItem::Type& val) {
  switch (val) {
    case blink::mojom::MenuItem::Type::kCheckableOption:
      return StringToV8(isolate, "checkbox");
    case blink::mojom::MenuItem::Type::kGroup:
      return StringToV8(isolate, "radio");
    case blink::mojom::MenuItem::Type::kSeparator:
      return StringToV8(isolate, "separator");
    case blink::mojom::MenuItem::Type::kSubMenu:
      return StringToV8(isolate, "submenu");
    case blink::mojom::MenuItem::Type::kOption:
    default:
      return StringToV8(isolate, "normal");
  }
}

// static
v8::Local<v8::Value> Converter<ContextMenuParamsWithRenderFrameHost>::ToV8(
    v8::Isolate* isolate,
    const ContextMenuParamsWithRenderFrameHost& val) {
  const auto& params = val.first;
  content::RenderFrameHost* render_frame_host = val.second;
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.SetGetter("frame", render_frame_host, v8::DontEnum);
  dict.Set("x", params.x);
  dict.Set("y", params.y);
  dict.Set("linkURL", params.link_url);
  dict.Set("linkText", params.link_text);
  dict.Set("pageURL", params.page_url);
  dict.Set("frameURL", params.frame_url);
  dict.Set("srcURL", params.src_url);
  dict.Set("mediaType", params.media_type);
  dict.Set("mediaFlags", MediaFlagsToV8(isolate, params.media_flags));
  bool has_image_contents =
      (params.media_type == blink::mojom::ContextMenuDataMediaType::kImage) &&
      params.has_image_contents;
  dict.Set("hasImageContents", has_image_contents);
  dict.Set("isEditable", params.is_editable);
  dict.Set("editFlags", EditFlagsToV8(isolate, params.edit_flags));
  dict.Set("selectionText", params.selection_text);
  dict.Set("titleText", params.title_text);
  dict.Set("altText", params.alt_text);
  dict.Set("suggestedFilename", params.suggested_filename);
  dict.Set("misspelledWord", params.misspelled_word);
  dict.Set("selectionRect", params.selection_rect);
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  dict.Set("dictionarySuggestions", params.dictionary_suggestions);
  dict.Set("spellcheckEnabled", params.spellcheck_enabled);
#else
  dict.Set("spellcheckEnabled", false);
#endif
  dict.Set("frameCharset", params.frame_charset);
  dict.Set("referrerPolicy", params.referrer_policy);
  dict.Set("inputFieldType", params.input_field_type);
  dict.Set("menuSourceType", params.source_type);

  return gin::ConvertToV8(isolate, dict);
}

// static
bool Converter<blink::mojom::PermissionStatus>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    blink::mojom::PermissionStatus* out) {
  bool result;
  if (!ConvertFromV8(isolate, val, &result))
    return false;

  if (result)
    *out = blink::mojom::PermissionStatus::GRANTED;
  else
    *out = blink::mojom::PermissionStatus::DENIED;

  return true;
}

// static
v8::Local<v8::Value> Converter<blink::PermissionType>::ToV8(
    v8::Isolate* isolate,
    const blink::PermissionType& val) {
  using PermissionType = electron::WebContentsPermissionHelper::PermissionType;
  // Based on mappings from content/browser/devtools/protocol/browser_handler.cc
  // Not all permissions are currently used by Electron but this will future
  // proof these conversions.
  switch (val) {
    case blink::PermissionType::ACCESSIBILITY_EVENTS:
      return StringToV8(isolate, "accessibility-events");
    case blink::PermissionType::AR:
      return StringToV8(isolate, "ar");
    case blink::PermissionType::BACKGROUND_FETCH:
      return StringToV8(isolate, "background-fetch");
    case blink::PermissionType::BACKGROUND_SYNC:
      return StringToV8(isolate, "background-sync");
    case blink::PermissionType::CLIPBOARD_READ_WRITE:
      return StringToV8(isolate, "clipboard-read");
    case blink::PermissionType::CLIPBOARD_SANITIZED_WRITE:
      return StringToV8(isolate, "clipboard-sanitized-write");
    case blink::PermissionType::LOCAL_FONTS:
      return StringToV8(isolate, "local-fonts");
    case blink::PermissionType::IDLE_DETECTION:
      return StringToV8(isolate, "idle-detection");
    case blink::PermissionType::MIDI_SYSEX:
      return StringToV8(isolate, "midiSysex");
    case blink::PermissionType::NFC:
      return StringToV8(isolate, "nfc");
    case blink::PermissionType::NOTIFICATIONS:
      return StringToV8(isolate, "notifications");
    case blink::PermissionType::PAYMENT_HANDLER:
      return StringToV8(isolate, "payment-handler");
    case blink::PermissionType::PERIODIC_BACKGROUND_SYNC:
      return StringToV8(isolate, "periodic-background-sync");
    case blink::PermissionType::DURABLE_STORAGE:
      return StringToV8(isolate, "persistent-storage");
    case blink::PermissionType::GEOLOCATION:
      return StringToV8(isolate, "geolocation");
    case blink::PermissionType::CAMERA_PAN_TILT_ZOOM:
    case blink::PermissionType::AUDIO_CAPTURE:
    case blink::PermissionType::VIDEO_CAPTURE:
      return StringToV8(isolate, "media");
    case blink::PermissionType::PROTECTED_MEDIA_IDENTIFIER:
      return StringToV8(isolate, "mediaKeySystem");
    case blink::PermissionType::MIDI:
      return StringToV8(isolate, "midi");
    case blink::PermissionType::WAKE_LOCK_SCREEN:
      return StringToV8(isolate, "screen-wake-lock");
    case blink::PermissionType::SENSORS:
      return StringToV8(isolate, "sensors");
    case blink::PermissionType::STORAGE_ACCESS_GRANT:
      return StringToV8(isolate, "storage-access");
    case blink::PermissionType::VR:
      return StringToV8(isolate, "vr");
    case blink::PermissionType::WAKE_LOCK_SYSTEM:
      return StringToV8(isolate, "system-wake-lock");
    case blink::PermissionType::WINDOW_PLACEMENT:
      return StringToV8(isolate, "window-placement");
    case blink::PermissionType::DISPLAY_CAPTURE:
      return StringToV8(isolate, "display-capture");
    case blink::PermissionType::NUM:
      break;
  }

  switch (static_cast<PermissionType>(val)) {
    case PermissionType::POINTER_LOCK:
      return StringToV8(isolate, "pointerLock");
    case PermissionType::FULLSCREEN:
      return StringToV8(isolate, "fullscreen");
    case PermissionType::OPEN_EXTERNAL:
      return StringToV8(isolate, "openExternal");
    case PermissionType::SERIAL:
      return StringToV8(isolate, "serial");
    case PermissionType::HID:
      return StringToV8(isolate, "hid");
    default:
      return StringToV8(isolate, "unknown");
  }
}

// static
bool Converter<content::StopFindAction>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                content::StopFindAction* out) {
  std::string action;
  if (!ConvertFromV8(isolate, val, &action))
    return false;

  if (action == "clearSelection")
    *out = content::STOP_FIND_ACTION_CLEAR_SELECTION;
  else if (action == "keepSelection")
    *out = content::STOP_FIND_ACTION_KEEP_SELECTION;
  else if (action == "activateSelection")
    *out = content::STOP_FIND_ACTION_ACTIVATE_SELECTION;
  else
    return false;

  return true;
}

// static
v8::Local<v8::Value> Converter<content::WebContents*>::ToV8(
    v8::Isolate* isolate,
    content::WebContents* val) {
  if (!val)
    return v8::Null(isolate);
  return electron::api::WebContents::FromOrCreate(isolate, val).ToV8();
}

// static
bool Converter<content::WebContents*>::FromV8(v8::Isolate* isolate,
                                              v8::Local<v8::Value> val,
                                              content::WebContents** out) {
  if (!val->IsObject())
    return false;
  // gin's unwrapping converter doesn't expect the pointer inside to ever be
  // nullptr, so we check here first before attempting to unwrap.
  if (gin_helper::Destroyable::IsDestroyed(val.As<v8::Object>()))
    return false;
  electron::api::WebContents* web_contents = nullptr;
  if (!gin::ConvertFromV8(isolate, val, &web_contents) || !web_contents)
    return false;

  *out = web_contents->web_contents();
  return true;
}

// static
v8::Local<v8::Value> Converter<content::Referrer>::ToV8(
    v8::Isolate* isolate,
    const content::Referrer& val) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("url", ConvertToV8(isolate, val.url));
  dict.Set("policy", ConvertToV8(isolate, val.policy));
  return gin::ConvertToV8(isolate, dict);
}

// static
bool Converter<content::Referrer>::FromV8(v8::Isolate* isolate,
                                          v8::Local<v8::Value> val,
                                          content::Referrer* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  if (!dict.Get("url", &out->url))
    return false;

  if (!dict.Get("policy", &out->policy))
    return false;

  return true;
}

// static
bool Converter<content::NativeWebKeyboardEvent>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    content::NativeWebKeyboardEvent* out) {
  gin_helper::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!ConvertFromV8(isolate, val, static_cast<blink::WebKeyboardEvent*>(out)))
    return false;
  dict.Get("skipInBrowser", &out->skip_in_browser);
  return true;
}

// static
v8::Local<v8::Value> Converter<content::NativeWebKeyboardEvent>::ToV8(
    v8::Isolate* isolate,
    const content::NativeWebKeyboardEvent& in) {
  return ConvertToV8(isolate, static_cast<blink::WebKeyboardEvent>(in));
}

}  // namespace gin
