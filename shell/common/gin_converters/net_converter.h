// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_H_

#include <string>

#include "gin/converter.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "shell/browser/net/cert_verifier_client.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

namespace net {
class AuthChallengeInfo;
class URLRequest;
class X509Certificate;
class HttpResponseHeaders;
struct CertPrincipal;
class HttpVersion;
}  // namespace net

namespace network {
struct ResourceRequest;
}

namespace gin {

template <>
struct Converter<net::AuthChallengeInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::AuthChallengeInfo& val);
};

template <>
struct Converter<scoped_refptr<net::X509Certificate>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<net::X509Certificate>& val);

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     scoped_refptr<net::X509Certificate>* out);
};

template <>
struct Converter<net::CertPrincipal> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::CertPrincipal& val);
};

template <>
struct Converter<net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   net::HttpResponseHeaders* headers);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::HttpResponseHeaders* out);
};

template <>
struct Converter<net::HttpRequestHeaders> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::HttpRequestHeaders& headers);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::HttpRequestHeaders* out);
};

template <>
struct Converter<network::ResourceRequestBody> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequestBody& val);
};

template <>
struct Converter<scoped_refptr<network::ResourceRequestBody>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<network::ResourceRequestBody>& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     scoped_refptr<network::ResourceRequestBody>* out);
};

template <>
struct Converter<network::ResourceRequest> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequest& val);
};

template <>
struct Converter<electron::VerifyRequestParams> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::VerifyRequestParams val);
};

template <>
struct Converter<net::HttpVersion> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::HttpVersion& val);
};

template <>
struct Converter<net::RedirectInfo> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::RedirectInfo& val);
};

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_NET_CONVERTER_H_
