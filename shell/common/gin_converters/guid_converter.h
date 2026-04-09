// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_

#include <string>

#include "base/strings/string_util.h"
#include "base/uuid.h"
#include "gin/converter.h"

#if BUILDFLAG(IS_WIN)
// c.f.:
// https://chromium-review.googlesource.com/c/chromium/src/+/3076480
// REFGUID is currently incorrectly inheriting its type
// from base::GUID, when it should be inheriting from ::GUID.
// This workaround prevents build errors until the CL is merged
#ifdef _REFGUID_DEFINED
#undef REFGUID
#endif
#define REFGUID const ::GUID&
#define _REFGUID_DEFINED

#include <rpc.h>

#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util.h"
#endif

#if BUILDFLAG(IS_WIN)
typedef GUID UUID;
#else
typedef struct {
} UUID;
#endif

namespace gin {

template <>
struct Converter<base::Uuid> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Uuid* out) {
    std::string guid;
    if (!gin::ConvertFromV8(isolate, val, &guid))
      return false;

    base::Uuid parsed = base::Uuid::ParseLowercase(base::ToLowerASCII(guid));
    if (!parsed.is_valid())
      return false;

    *out = parsed;
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, base::Uuid val) {
    const std::string guid = val.AsLowercaseString();
    return gin::ConvertToV8(isolate, guid);
  }
};

#if BUILDFLAG(IS_WIN)
template <>
struct Converter<UUID> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     UUID* out) {
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
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, UUID val) {
    const GUID GUID_NULL = {};
    if (val == GUID_NULL) {
      return v8::String::Empty(isolate);
    } else {
      std::wstring uid = base::win::WStringFromGUID(val);
      return StringToV8(isolate, base::SysWideToUTF8(uid));
    }
  }
};
#endif

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_CONVERTERS_GUID_CONVERTER_H_
