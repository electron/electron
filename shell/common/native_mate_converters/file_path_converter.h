// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_

#include <string>

#include "base/files/file_path.h"
#include "shell/common/native_mate_converters/string16_converter.h"

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

    v8::String::Value str(isolate, val);
    if (str.length() == 0) {
      *out = base::FilePath();
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
