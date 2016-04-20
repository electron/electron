// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/content_converter.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "native_mate/dictionary.h"

namespace {

void ExecuteCommand(content::WebContents* web_contents,
                    int action,
                    const content::CustomContextMenuContext& context) {
  web_contents->ExecuteCustomContextMenuCommand(action, context);
}

// Forward declaration for nested recursive call.
v8::Local<v8::Value> MenuToV8(v8::Isolate* isolate,
                              content::WebContents* web_contents,
                              const content::CustomContextMenuContext& context,
                              const std::vector<content::MenuItem>& menu);

v8::Local<v8::Value> MenuItemToV8(
    v8::Isolate* isolate,
    content::WebContents* web_contents,
    const content::CustomContextMenuContext& context,
    const content::MenuItem& item) {
  mate::Dictionary v8_item = mate::Dictionary::CreateEmpty(isolate);
  switch (item.type) {
    case content::MenuItem::CHECKABLE_OPTION:
    case content::MenuItem::GROUP:
      v8_item.Set("checked", item.checked);
    case content::MenuItem::OPTION:
    case content::MenuItem::SUBMENU:
      v8_item.Set("label", item.label);
      v8_item.Set("enabled", item.enabled);
    default:
      v8_item.Set("type", item.type);
  }
  if (item.type == content::MenuItem::SUBMENU)
    v8_item.Set("submenu",
                MenuToV8(isolate, web_contents, context, item.submenu));
  else if (item.action > 0)
    v8_item.Set("click",
                base::Bind(ExecuteCommand, web_contents, item.action, context));
  return v8_item.GetHandle();
}

v8::Local<v8::Value> MenuToV8(v8::Isolate* isolate,
                              content::WebContents* web_contents,
                              const content::CustomContextMenuContext& context,
                              const std::vector<content::MenuItem>& menu) {
  std::vector<v8::Local<v8::Value>> v8_menu;
  for (const auto& menu_item : menu)
    v8_menu.push_back(MenuItemToV8(isolate, web_contents, context, menu_item));
  return mate::ConvertToV8(isolate, v8_menu);
}

}  // namespace

namespace mate {

// static
v8::Local<v8::Value> Converter<content::MenuItem::Type>::ToV8(
    v8::Isolate* isolate, const content::MenuItem::Type& val) {
  switch (val) {
    case content::MenuItem::CHECKABLE_OPTION:
      return StringToV8(isolate, "checkbox");
    case content::MenuItem::GROUP:
      return StringToV8(isolate, "radio");
    case content::MenuItem::SEPARATOR:
      return StringToV8(isolate, "separator");
    case content::MenuItem::SUBMENU:
      return StringToV8(isolate, "submenu");
    case content::MenuItem::OPTION:
    default:
      return StringToV8(isolate, "normal");
  }
}

// static
v8::Local<v8::Value> Converter<ContextMenuParamsWithWebContents>::ToV8(
    v8::Isolate* isolate, const ContextMenuParamsWithWebContents& val) {
  const auto& params = val.first;
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("x", params.x);
  dict.Set("y", params.y);
  if (params.custom_context.is_pepper_menu)
    dict.Set("menu", MenuToV8(isolate, val.second, params.custom_context,
                              params.custom_items));
  return mate::ConvertToV8(isolate, dict);
}

// static
bool Converter<content::PermissionStatus>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    content::PermissionStatus* out) {
  bool result;
  if (!ConvertFromV8(isolate, val, &result))
    return false;

  if (result)
    *out = content::PERMISSION_STATUS_GRANTED;
  else
    *out = content::PERMISSION_STATUS_DENIED;

  return true;
}

// static
v8::Local<v8::Value> Converter<content::PermissionType>::ToV8(
    v8::Isolate* isolate, const content::PermissionType& val) {
  using PermissionType = atom::WebContentsPermissionHelper::PermissionType;
  switch (val) {
    case content::PermissionType::MIDI_SYSEX:
      return StringToV8(isolate, "midiSysex");
    case content::PermissionType::PUSH_MESSAGING:
      return StringToV8(isolate, "pushMessaging");
    case content::PermissionType::NOTIFICATIONS:
      return StringToV8(isolate, "notifications");
    case content::PermissionType::GEOLOCATION:
      return StringToV8(isolate, "geolocation");
    case content::PermissionType::AUDIO_CAPTURE:
    case content::PermissionType::VIDEO_CAPTURE:
      return StringToV8(isolate, "media");
    case content::PermissionType::PROTECTED_MEDIA_IDENTIFIER:
      return StringToV8(isolate, "mediaKeySystem");
    case content::PermissionType::MIDI:
      return StringToV8(isolate, "midi");
    default:
      break;
  }

  if (val == (content::PermissionType)(PermissionType::POINTER_LOCK))
    return StringToV8(isolate, "pointerLock");
  else if (val == (content::PermissionType)(PermissionType::FULLSCREEN))
    return StringToV8(isolate, "fullscreen");
  else if (val == (content::PermissionType)(PermissionType::OPEN_EXTERNAL))
    return StringToV8(isolate, "openExternal");

  return StringToV8(isolate, "unknown");
}

// static
bool Converter<content::StopFindAction>::FromV8(
    v8::Isolate* isolate,
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
    v8::Isolate* isolate, content::WebContents* val) {
  if (!val)
    return v8::Null(isolate);
  return atom::api::WebContents::CreateFrom(isolate, val).ToV8();
}

}  // namespace mate
