// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_

#if defined(OS_WIN)
#include <rpc.h>

#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util.h"
#endif
#include <string>

#include "gin/converter.h"

#if defined(OS_WIN)
typedef GUID UUID;
const GUID GUID_NULL = {};
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

    if (!guid.empty()) {
      if (guid[0] == '{' && guid[guid.length() - 1] == '}') {
        guid = guid.substr(1, guid.length() - 2);
      }
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
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, UUID val) {
#if defined(OS_WIN)
    if (val == GUID_NULL) {
      return StringToV8(isolate, "");
    } else {
      std::wstring uid = base::win::WStringFromGUID(val);
      return StringToV8(isolate, base::SysWideToUTF8(uid));
    }
#else
    return v8::Undefined(isolate);
#endif
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
