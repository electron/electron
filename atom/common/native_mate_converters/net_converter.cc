// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/net_converter.h"

#include <string>

#include "atom/common/node_includes.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "native_mate/dictionary.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace mate {

// static
v8::Local<v8::Value> Converter<const net::URLRequest*>::ToV8(
    v8::Isolate* isolate, const net::URLRequest* val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("method", val->method());
  dict.Set("url", val->url().spec());
  dict.Set("referrer", val->referrer());
  return mate::ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<const net::AuthChallengeInfo*>::ToV8(
    v8::Isolate* isolate, const net::AuthChallengeInfo* val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("isProxy", val->is_proxy);
  dict.Set("scheme", val->scheme);
  dict.Set("host", val->challenger.host());
  dict.Set("port", static_cast<uint32_t>(val->challenger.port()));
  dict.Set("realm", val->realm);
  return mate::ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<scoped_refptr<net::X509Certificate>>::ToV8(
    v8::Isolate* isolate, const scoped_refptr<net::X509Certificate>& val) {
  mate::Dictionary dict(isolate, v8::Object::New(isolate));
  std::string encoded_data;
  net::X509Certificate::GetPEMEncoded(
      val->os_cert_handle(), &encoded_data);
  auto buffer = node::Buffer::Copy(isolate,
                                   encoded_data.data(),
                                   encoded_data.size()).ToLocalChecked();
  dict.Set("data", buffer);
  dict.Set("issuerName", val->issuer().GetDisplayName());
  return dict.GetHandle();
}

// static
bool Converter<atom::AtomNetworkDelegate::BlockingResponse>::FromV8(
    v8::Isolate* isolate, v8::Local<v8::Value> val,
    atom::AtomNetworkDelegate::BlockingResponse* out) {
  mate::Dictionary dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  if (!dict.Get("cancel", &(out->cancel)))
    return false;
  dict.Get("redirectURL", &(out->redirectURL));
  base::DictionaryValue request_headers;
  if (dict.Get("requestHeaders", &request_headers)) {
    for (base::DictionaryValue::Iterator it(request_headers);
         !it.IsAtEnd();
         it.Advance()) {
      std::string value;
      CHECK(it.value().GetAsString(&value));
      out->requestHeaders.SetHeader(it.key(), value);
    }
  }
  base::DictionaryValue response_headers;
  if (dict.Get("responseHeaders", &response_headers)) {
    out->responseHeaders = new net::HttpResponseHeaders("");
    for (base::DictionaryValue::Iterator it(response_headers);
         !it.IsAtEnd();
         it.Advance()) {
      std::string value;
      CHECK(it.value().GetAsString(&value));
      out->responseHeaders->AddHeader(it.key() + " : " + value);
    }
  }
  return true;
}

}  // namespace mate
