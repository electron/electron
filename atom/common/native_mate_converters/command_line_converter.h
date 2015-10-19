// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_COMMAND_LINE_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_COMMAND_LINE_CONVERTER_H_

#include <string>

#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/command_line.h"

namespace mate {

template<>
struct Converter<base::CommandLine> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    const base::CommandLine& val) {
    return Converter<base::CommandLine::StringType>::ToV8(isolate, val.GetCommandLineString());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::CommandLine* out) {
    base::FilePath::StringType path;
    
    if (Converter<base::FilePath::StringType>::FromV8(isolate, val, &path)) {
      *out = base::CommandLine(base::FilePath(path));
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_FILE_PATH_CONVERTER_H_
