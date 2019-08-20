// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_

#include "base/files/file_path.h"
#include "gin/converter.h"

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

#endif  // SHELL_COMMON_GIN_CONVERTERS_FILE_PATH_CONVERTER_H_
