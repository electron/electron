// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_

#include "base/files/file_path.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/std_converter.h"

namespace gin {

template <>
struct Converter<base::FilePath> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::FilePath& val) {
    return Converter<base::FilePath::StringType>::ToV8(isolate, val.value());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::FilePath* out) {
    if (val->IsNull())
      return true;

    if (val->IsString() && v8::Local<v8::String>::Cast(val)->Length() == 0) {
      *out = {};
      return true;
    }

    base::FilePath::StringType path;
    if (Converter<base::FilePath::StringType>::FromV8(isolate, val, &path)) {
      *out = base::FilePath(path);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
