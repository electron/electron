// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_

#include <string>

#include "gin/converter.h"
#include "shell/common/native_mate_converters/net_converter.h"

namespace gin {

template <>
struct Converter<net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   net::HttpResponseHeaders* headers) {
    return mate::ConvertToV8(isolate, headers);
  }
};

template <>
struct Converter<network::ResourceRequest> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequest& val) {
    return mate::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<network::ResourceRequestBody> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequestBody& val) {
    return mate::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<net::HttpRequestHeaders> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::HttpRequestHeaders& val) {
    return mate::ConvertToV8(isolate, val);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::HttpRequestHeaders* out) {
    base::DictionaryValue dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    for (base::DictionaryValue::Iterator it(dict); !it.IsAtEnd();
         it.Advance()) {
      if (it.value().is_string()) {
        std::string value = it.value().GetString();
        out->SetHeader(it.key(), value);
      }
    }
    return true;
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_GIN_ADAPTER_H_
