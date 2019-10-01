// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_

#include "shell/common/gin_converters/file_path_converter.h"

namespace mate {

template <>
struct Converter<base::FilePath> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::FilePath& val) {
    return gin::Converter<base::FilePath::StringType>::ToV8(isolate,
                                                            val.value());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::FilePath* out) {
    return gin::Converter<base::FilePath>::FromV8(isolate, val, out);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_
