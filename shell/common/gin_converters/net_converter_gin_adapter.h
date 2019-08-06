// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_

#include "gin/converter.h"
#include "shell/common/native_mate_converters/net_converter.h"

// TODO(deermichel): replace adapter with real implementation after removing
// mate
// -- this adapter forwards all conversions to the existing mate converter --
// (other direction might be preferred, but this is safer for now :D)

namespace gin {

template <>
struct Converter<scoped_refptr<net::X509Certificate>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<net::X509Certificate>& val) {
    return mate::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     scoped_refptr<net::X509Certificate>* out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_
