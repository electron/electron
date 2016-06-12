// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/files/file_path.h"

namespace mate {

template<>
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

}  // namespace mate
