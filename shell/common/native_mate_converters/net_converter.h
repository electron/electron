// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_

#include "native_mate/converter.h"
#include "shell/common/gin_converters/net_converter.h"

namespace mate {

template <>
struct Converter<net::AuthChallengeInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::AuthChallengeInfo& val) {
    return gin::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<scoped_refptr<net::X509Certificate>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<net::X509Certificate>& val) {
    return gin::ConvertToV8(isolate, val);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     scoped_refptr<net::X509Certificate>* out) {
    return gin::ConvertFromV8(isolate, val, out);
  }
};

template <>
struct Converter<net::CertPrincipal> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::CertPrincipal& val) {
    return gin::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   net::HttpResponseHeaders* headers) {
    return gin::ConvertToV8(isolate, headers);
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::HttpResponseHeaders* out) {
    return gin::Converter<net::HttpResponseHeaders*>::FromV8(isolate, val, out);
  }
};

template <>
struct Converter<network::ResourceRequest> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequest& val) {
    return gin::ConvertToV8(isolate, val);
  }
};

template <>
struct Converter<electron::VerifyRequestParams> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::VerifyRequestParams val) {
    return gin::ConvertToV8(isolate, val);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
