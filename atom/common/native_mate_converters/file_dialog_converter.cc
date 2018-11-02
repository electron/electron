// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/file_dialog_converter.h"

#include "atom/browser/api/atom_api_browser_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "native_mate/dictionary.h"

namespace mate {

bool Converter<file_dialog::Filter>::FromV8(v8::Isolate* isolate,
                                            v8::Local<v8::Value> val,
                                            file_dialog::Filter* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!dict.Get("name", &(out->first)))
    return false;
  if (!dict.Get("extensions", &(out->second)))
    return false;
  return true;
}

v8::Local<v8::Value> Converter<file_dialog::Filter>::ToV8(
    v8::Isolate* isolate,
    const file_dialog::Filter& in) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);

  dict.Set("name", in.first);
  dict.Set("extensions", in.second);

  return dict.GetHandle();
}

bool Converter<file_dialog::DialogSettings>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    file_dialog::DialogSettings* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  dict.Get("window", &(out->parent_window));
  dict.Get("title", &(out->title));
  dict.Get("message", &(out->message));
  dict.Get("buttonLabel", &(out->button_label));
  dict.Get("nameFieldLabel", &(out->name_field_label));
  dict.Get("defaultPath", &(out->default_path));
  dict.Get("filters", &(out->filters));
  dict.Get("properties", &(out->properties));
  dict.Get("showsTagField", &(out->shows_tag_field));
  dict.Get("securityScopedBookmarks", &(out->security_scoped_bookmarks));
  return true;
}

v8::Local<v8::Value> Converter<file_dialog::DialogSettings>::ToV8(
    v8::Isolate* isolate,
    const file_dialog::DialogSettings& in) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);

  dict.Set("window", atom::api::BrowserWindow::From(isolate, in.parent_window));
  dict.Set("title", in.title);
  dict.Set("message", in.message);
  dict.Set("buttonLabel", in.button_label);
  dict.Set("nameFieldLabel", in.name_field_label);
  dict.Set("defaultPath", in.default_path);
  dict.Set("filters", in.filters);
  dict.Set("showsTagField", in.shows_tag_field);
  dict.Set("securityScopedBookmarks", in.security_scoped_bookmarks);

  return dict.GetHandle();
}

}  // namespace mate
