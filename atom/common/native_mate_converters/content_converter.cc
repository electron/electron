// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/content_converter.h"

#include <vector>

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

}  // namespace mate
