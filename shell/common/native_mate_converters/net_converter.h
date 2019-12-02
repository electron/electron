// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_

#include "base/memory/ref_counted.h"
#include "native_mate/converter.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

namespace electron {
struct VerifyRequestParams;
}

namespace net {
class AuthChallengeInfo;
class URLRequest;
class X509Certificate;
class HttpResponseHeaders;
class HttpRequestHeaders;
struct CertPrincipal;
class HttpVersion;
struct RedirectInfo;
}  // namespace net

namespace network {
class ResourceRequestBody;
struct ResourceRequest;
}  // namespace network

namespace mate {

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
};

template <>
struct Converter<network::ResourceRequestBody> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const network::ResourceRequestBody& val);
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

}  // namespace mate

namespace electron {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request);

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request);

}  // namespace electron

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
