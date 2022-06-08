// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_H_

#include <string>

#include "gin/converter.h"
#include "url/gurl.h"

namespace gin {

template <>
struct Converter<GURL> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const GURL& val) {
    return ConvertToV8(isolate, val.spec());
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     GURL* out) {
    std::string url;
    if (Converter<std::string>::FromV8(isolate, val, &url)) {
      *out = GURL(url);
      return true;
    } else {
      return false;
    }
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GURL_CONVERTER_H_
