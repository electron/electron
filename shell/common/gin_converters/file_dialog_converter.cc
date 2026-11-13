// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/file_dialog_converter.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "gin/dictionary.h"
#include "shell/browser/api/electron_api_browser_window.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/native_window_converter.h"
#include "shell/common/gin_converters/std_converter.h"

namespace file_dialog {

namespace {

template <typename Map>
int PropertiesFromV8(v8::Isolate* isolate,
                     const std::vector<v8::Local<v8::Value>>& names,
                     const Map& map) {
  int properties = 0;
  for (const auto& name : names) {
    if (!name->IsString())
      continue;
    if (const int* flag = base::FindOrNull(map, gin::V8ToString(isolate, name)))
      properties |= *flag;
  }
  return properties;
}

}  // namespace

int OpenDialogPropertiesFromV8(v8::Isolate* isolate,
                               const std::vector<v8::Local<v8::Value>>& names) {
  static constexpr auto kFlags = base::MakeFixedFlatMap<std::string_view, int>({
      {"openFile", OPEN_DIALOG_OPEN_FILE},
      {"openDirectory", OPEN_DIALOG_OPEN_DIRECTORY},
      {"multiSelections", OPEN_DIALOG_MULTI_SELECTIONS},
      {"createDirectory", OPEN_DIALOG_CREATE_DIRECTORY},
      {"showHiddenFiles", OPEN_DIALOG_SHOW_HIDDEN_FILES},
      {"promptToCreate", OPEN_DIALOG_PROMPT_TO_CREATE},
      {"noResolveAliases", OPEN_DIALOG_NO_RESOLVE_ALIASES},
      {"treatPackageAsDirectory", OPEN_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY},
      {"dontAddToRecent", FILE_DIALOG_DONT_ADD_TO_RECENT},
  });
  return PropertiesFromV8(isolate, names, kFlags);
}

constexpr auto kSaveDialogFlags =
    base::MakeFixedFlatMap<std::string_view, int>({
        {"createDirectory", SAVE_DIALOG_CREATE_DIRECTORY},
        {"showHiddenFiles", SAVE_DIALOG_SHOW_HIDDEN_FILES},
        {"treatPackageAsDirectory", SAVE_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY},
        {"showOverwriteConfirmation", SAVE_DIALOG_SHOW_OVERWRITE_CONFIRMATION},
        {"dontAddToRecent", SAVE_DIALOG_DONT_ADD_TO_RECENT},
    });

int SaveDialogPropertiesFromV8(v8::Isolate* isolate,
                               const std::vector<v8::Local<v8::Value>>& names) {
  return PropertiesFromV8(isolate, names, kSaveDialogFlags);
}

std::vector<std::string_view> SaveDialogPropertyNames(int properties) {
  std::vector<std::string_view> names;
  for (const auto& [name, flag] : kSaveDialogFlags) {
    if (properties & flag)
      names.push_back(name);
  }
  return names;
}

}  // namespace file_dialog

namespace gin {

bool Converter<file_dialog::Filter>::FromV8(v8::Isolate* isolate,
                                            v8::Local<v8::Value> val,
                                            file_dialog::Filter* out) {
  gin::Dictionary dict(nullptr);
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
  auto dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("name", in.first);
  dict.Set("extensions", in.second);

  return gin::ConvertToV8(isolate, dict);
}

bool Converter<file_dialog::DialogSettings>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    file_dialog::DialogSettings* out) {
  gin::Dictionary dict(nullptr);
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  dict.Get("window", &(out->parent_window));
  dict.Get("title", &(out->title));
  dict.Get("message", &(out->message));
  dict.Get("buttonLabel", &(out->button_label));
  dict.Get("nameFieldLabel", &(out->name_field_label));
  dict.Get("defaultPath", &(out->default_path));
  dict.Get("filters", &(out->filters));
  std::vector<v8::Local<v8::Value>> properties;
  if (dict.Get("properties", &properties)) {
    out->properties =
        file_dialog::SaveDialogPropertiesFromV8(isolate, properties);
  }
  dict.Get("showsTagField", &(out->shows_tag_field));
  dict.Get("securityScopedBookmarks", &(out->security_scoped_bookmarks));
  return true;
}

v8::Local<v8::Value> Converter<file_dialog::DialogSettings>::ToV8(
    v8::Isolate* isolate,
    const file_dialog::DialogSettings& in) {
  auto dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("window",
           electron::api::BrowserWindow::From(isolate, in.parent_window));
  dict.Set("title", in.title);
  dict.Set("message", in.message);
  dict.Set("buttonLabel", in.button_label);
  dict.Set("nameFieldLabel", in.name_field_label);
  dict.Set("defaultPath", in.default_path);
  dict.Set("filters", in.filters);
  dict.Set("properties", file_dialog::SaveDialogPropertyNames(in.properties));
  dict.Set("showsTagField", in.shows_tag_field);
  dict.Set("securityScopedBookmarks", in.security_scoped_bookmarks);

  return gin::ConvertToV8(isolate, dict);
}

}  // namespace gin
