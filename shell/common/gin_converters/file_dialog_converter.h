// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_

#include <string_view>
#include <vector>

#include "gin/converter.h"
#include "shell/browser/ui/file_dialog.h"

namespace file_dialog {

// The OpenDialogProperty / SaveFileDialogProperty flags for a `properties`
// array of names; unknown entries are ignored.
int OpenDialogPropertiesFromV8(v8::Isolate* isolate,
                               const std::vector<v8::Local<v8::Value>>& names);
int SaveDialogPropertiesFromV8(v8::Isolate* isolate,
                               const std::vector<v8::Local<v8::Value>>& names);
// The names of the SaveFileDialogProperty flags set in |properties|.
std::vector<std::string_view> SaveDialogPropertyNames(int properties);

}  // namespace file_dialog

namespace gin {

template <>
struct Converter<file_dialog::Filter> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::Filter& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::Filter* out);
};

template <>
struct Converter<file_dialog::DialogSettings> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::DialogSettings& in);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::DialogSettings* out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_DIALOG_CONVERTER_H_
