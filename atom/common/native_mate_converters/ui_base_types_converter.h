// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_UI_BASE_TYPES_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_UI_BASE_TYPES_CONVERTER_H_

#include "native_mate/converter.h"
#include "ui/base/ui_base_types.h"

namespace mate {

template<>
struct Converter<ui::MenuSourceType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const ui::MenuSourceType& in) {
    switch (in) {
      case ui::MENU_SOURCE_MOUSE:
        return mate::StringToV8(isolate, "mouse");
      case ui::MENU_SOURCE_KEYBOARD:
        return mate::StringToV8(isolate, "keyboard");
      case ui::MENU_SOURCE_TOUCH:
        return mate::StringToV8(isolate, "touch");
      case ui::MENU_SOURCE_TOUCH_EDIT_MENU:
        return mate::StringToV8(isolate, "touch-menu");
      default:
        return mate::StringToV8(isolate, "none");
    }
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_UI_BASE_TYPES_CONVERTER_H_
