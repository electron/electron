// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/content_converter.h"

#include "atom/common/native_mate_converters/string16_converter.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/common/menu_item.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<content::MenuItem::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const content::MenuItem::Type& val) {
    switch (val) {
      case content::MenuItem::CHECKABLE_OPTION:
        return StringToV8(isolate, "checkbox");
      case content::MenuItem::SEPARATOR:
        return StringToV8(isolate, "separator");
      case content::MenuItem::SUBMENU:
        return StringToV8(isolate, "submenu");
      default:
        return StringToV8(isolate, "normal");
    }
  }
};

// static
v8::Local<v8::Value> Converter<content::MenuItem>::ToV8(
    v8::Isolate* isolate, const content::MenuItem& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("id", val.action);
  dict.Set("type", val.type);
  dict.Set("label", val.label);
  dict.Set("enabled", val.enabled);
  dict.Set("checked", val.checked);
  return mate::ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<content::ContextMenuParams>::ToV8(
    v8::Isolate* isolate, const content::ContextMenuParams& val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("x", val.x);
  dict.Set("y", val.y);
  dict.Set("isPepperMenu", val.custom_context.is_pepper_menu);
  dict.Set("menuItems", val.custom_items);
  return mate::ConvertToV8(isolate, dict);
}

}  // namespace mate
