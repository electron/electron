// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/native_mate_converters/message_box_converter.h"

#include "native_mate/dictionary.h"
#include "shell/browser/api/atom_api_browser_window.h"
#include "shell/common/native_mate_converters/file_path_converter.h"
#include "shell/common/native_mate_converters/image_converter.h"

namespace mate {

bool Converter<electron::MessageBoxSettings>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    electron::MessageBoxSettings* out) {
  mate::Dictionary dict;
  int type = 0;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  dict.Get("window", &out->parent_window);
  dict.Get("messageBoxType", &type);
  out->type = static_cast<electron::MessageBoxType>(type);
  dict.Get("buttons", &out->buttons);
  dict.Get("defaultId", &out->default_id);
  dict.Get("cancelId", &out->cancel_id);
  dict.Get("title", &out->title);
  dict.Get("message", &out->message);
  dict.Get("detail", &out->detail);
  dict.Get("checkboxLabel", &out->checkbox_label);
  dict.Get("noLink", &out->no_link);
  dict.Get("checkboxChecked", &out->checkbox_checked);
  dict.Get("icon", &out->icon);
  return true;
}

}  // namespace mate
