// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_DIALOG_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_DIALOG_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/common/gin_converters/file_dialog_converter.h"

namespace mate {

template <>
struct Converter<file_dialog::Filter> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::Filter& in) {
    return gin::ConvertToV8(isolate, in);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::Filter* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<file_dialog::DialogSettings> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const file_dialog::DialogSettings& in) {
    return gin::ConvertToV8(isolate, in);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::DialogSettings* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_DIALOG_CONVERTER_H_
