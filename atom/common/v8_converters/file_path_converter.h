// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_V8_CONVERTERS_FILE_PATH_CONVERTER_H_
#define ATOM_COMMON_V8_CONVERTERS_FILE_PATH_CONVERTER_H_

#include "atom/common/v8_converters/string16_converter.h"
#include "base/files/file_path.h"

namespace mate {

template<>
struct Converter<base::FilePath> {
  static v8::Handle<v8::Value> ToV8(v8::Isolate* isolate,
                                    const base::FilePath& val) {
    return Converter<base::FilePath::StringType>::ToV8(isolate, val.value());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Handle<v8::Value> val,
                     base::FilePath* out) {
    std::string path;
    if (Converter<std::string>::FromV8(isolate, val, &path)) {
      *out = base::FilePath::FromUTF8Unsafe(path);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_V8_CONVERTERS_FILE_PATH_CONVERTER_H_
