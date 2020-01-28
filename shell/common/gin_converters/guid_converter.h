// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_

#if defined(OS_WIN)
#include <rpc.h>
#endif
#include <string>

#include "gin/converter.h"

#if defined(OS_WIN)
typedef GUID UUID;
#else
typedef struct {
} UUID;
#endif

namespace gin {

template <>
struct Converter<UUID> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     UUID* out) {
#if defined(OS_WIN)
    std::string guid;
    if (!gin::ConvertFromV8(isolate, val, &guid))
      return false;

    UUID uid;

    if (guid.length() > 0) {
      unsigned char* uid_cstr = (unsigned char*)guid.c_str();
      RPC_STATUS result = UuidFromStringA(uid_cstr, &uid);
      if (result == RPC_S_INVALID_STRING_UUID) {
        return false;
      } else {
        *out = uid;
        return true;
      }
    }
    return false;
#else
    return false;
#endif
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
