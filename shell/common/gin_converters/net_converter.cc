// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_converters/net_converter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_version.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "shell/browser/api/electron_api_data_pipe_holder.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/node_includes.h"

namespace gin {

namespace {

bool CertFromData(const std::string& data,
                  scoped_refptr<net::X509Certificate>* out) {
  auto cert_list = net::X509Certificate::CreateCertificateListFromBytes(
      base::as_bytes(base::make_span(data)),
      net::X509Certificate::FORMAT_SINGLE_CERTIFICATE);
  if (cert_list.empty())
    return false;

  auto leaf_cert = cert_list.front();
  if (!leaf_cert)
    return false;

  *out = leaf_cert;

  return true;
}

}  // namespace

// static
v8::Local<v8::Value> Converter<net::AuthChallengeInfo>::ToV8(
    v8::Isolate* isolate,
    const net::AuthChallengeInfo& val) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("isProxy", val.is_proxy);
  dict.Set("scheme", val.scheme);
  dict.Set("host", val.challenger.host());
  dict.Set("port", static_cast<uint32_t>(val.challenger.port()));
  dict.Set("realm", val.realm);
  return gin::ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<scoped_refptr<net::X509Certificate>>::ToV8(
    v8::Isolate* isolate,
    const scoped_refptr<net::X509Certificate>& val) {
  gin::Dictionary dict(isolate, v8::Object::New(isolate));
  std::string encoded_data;
  net::X509Certificate::GetPEMEncoded(val->cert_buffer(), &encoded_data);

  dict.Set("data", encoded_data);
  dict.Set("issuer", val->issuer());
  dict.Set("issuerName", val->issuer().GetDisplayName());
  dict.Set("subject", val->subject());
  dict.Set("subjectName", val->subject().GetDisplayName());
  dict.Set("serialNumber", base::HexEncode(val->serial_number().data(),
                                           val->serial_number().size()));
  dict.Set("validStart", val->valid_start().ToDoubleT());
  dict.Set("validExpiry", val->valid_expiry().ToDoubleT());
  dict.Set("fingerprint",
           net::HashValue(val->CalculateFingerprint256(val->cert_buffer()))
               .ToString());

  const auto& intermediate_buffers = val->intermediate_buffers();
  if (!intermediate_buffers.empty()) {
    std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> issuer_intermediates;
    issuer_intermediates.reserve(intermediate_buffers.size() - 1);
    for (size_t i = 1; i < intermediate_buffers.size(); ++i) {
      issuer_intermediates.push_back(
          bssl::UpRef(intermediate_buffers[i].get()));
    }
    const scoped_refptr<net::X509Certificate>& issuer_cert =
        net::X509Certificate::CreateFromBuffer(
            bssl::UpRef(intermediate_buffers[0].get()),
            std::move(issuer_intermediates));
    dict.Set("issuerCert", issuer_cert);
  }

  return ConvertToV8(isolate, dict);
}

bool Converter<scoped_refptr<net::X509Certificate>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    scoped_refptr<net::X509Certificate>* out) {
  gin::Dictionary dict(nullptr);
  if (!ConvertFromV8(isolate, val, &dict))
    return false;

  std::string data;
  dict.Get("data", &data);
  scoped_refptr<net::X509Certificate> leaf_cert;
  if (!CertFromData(data, &leaf_cert))
    return false;

  scoped_refptr<net::X509Certificate> issuer_cert;
  if (dict.Get("issuerCert", &issuer_cert)) {
    std::vector<bssl::UniquePtr<CRYPTO_BUFFER>> intermediates;
    intermediates.push_back(bssl::UpRef(issuer_cert->cert_buffer()));
    auto cert = net::X509Certificate::CreateFromBuffer(
        bssl::UpRef(leaf_cert->cert_buffer()), std::move(intermediates));
    if (!cert)
      return false;

    *out = cert;
  } else {
    *out = leaf_cert;
  }

  return true;
}

// static
v8::Local<v8::Value> Converter<net::CertPrincipal>::ToV8(
    v8::Isolate* isolate,
    const net::CertPrincipal& val) {
  gin::Dictionary dict(isolate, v8::Object::New(isolate));

  dict.Set("commonName", val.common_name);
  dict.Set("organizations", val.organization_names);
  dict.Set("organizationUnits", val.organization_unit_names);
  dict.Set("locality", val.locality_name);
  dict.Set("state", val.state_or_province_name);
  dict.Set("country", val.country_name);

  return ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<net::HttpResponseHeaders*>::ToV8(
    v8::Isolate* isolate,
    net::HttpResponseHeaders* headers) {
  base::DictionaryValue response_headers;
  if (headers) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
      key = base::ToLowerASCII(key);
      base::Value* values = response_headers.FindListKey(key);
      if (!values)
        values = response_headers.SetKey(key, base::ListValue());
      values->Append(value);
    }
  }
  return ConvertToV8(isolate, response_headers);
}

bool Converter<net::HttpResponseHeaders*>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    net::HttpResponseHeaders* out) {
  if (!val->IsObject()) {
    return false;
  }

  auto addHeaderFromValue = [&isolate, &out](
                                const std::string& key,
                                const v8::Local<v8::Value>& localVal) {
    auto context = isolate->GetCurrentContext();
    v8::Local<v8::String> localStrVal;
    if (!localVal->ToString(context).ToLocal(&localStrVal)) {
      return false;
    }
    std::string value;
    gin::ConvertFromV8(isolate, localStrVal, &value);
    out->AddHeader(key, value);
    return true;
  };

  auto context = isolate->GetCurrentContext();
  auto headers = val.As<v8::Object>();
  auto keys = headers->GetOwnPropertyNames(context).ToLocalChecked();
  for (uint32_t i = 0; i < keys->Length(); i++) {
    v8::Local<v8::Value> keyVal;
    if (!keys->Get(context, i).ToLocal(&keyVal)) {
      return false;
    }
    std::string key;
    gin::ConvertFromV8(isolate, keyVal, &key);

    auto localVal = headers->Get(context, keyVal).ToLocalChecked();
    if (localVal->IsArray()) {
      auto values = localVal.As<v8::Array>();
      for (uint32_t j = 0; j < values->Length(); j++) {
        if (!addHeaderFromValue(key,
                                values->Get(context, j).ToLocalChecked())) {
          return false;
        }
      }
    } else {
      if (!addHeaderFromValue(key, localVal)) {
        return false;
      }
    }
  }
  return true;
}

// static
v8::Local<v8::Value> Converter<net::HttpRequestHeaders>::ToV8(
    v8::Isolate* isolate,
    const net::HttpRequestHeaders& val) {
  gin::Dictionary headers(isolate, v8::Object::New(isolate));
  for (net::HttpRequestHeaders::Iterator it(val); it.GetNext();)
    headers.Set(it.name(), it.value());
  return ConvertToV8(isolate, headers);
}

// static
bool Converter<net::HttpRequestHeaders>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                net::HttpRequestHeaders* out) {
  base::DictionaryValue dict;
  if (!ConvertFromV8(isolate, val, &dict))
    return false;
  for (base::DictionaryValue::Iterator it(dict); !it.IsAtEnd(); it.Advance()) {
    if (it.value().is_string()) {
      std::string value = it.value().GetString();
      out->SetHeader(it.key(), value);
    }
  }
  return true;
}

// static
v8::Local<v8::Value> Converter<network::ResourceRequestBody>::ToV8(
    v8::Isolate* isolate,
    const network::ResourceRequestBody& val) {
  const auto& elements = *val.elements();
  v8::Local<v8::Array> arr = v8::Array::New(isolate, elements.size());
  for (size_t i = 0; i < elements.size(); ++i) {
    const auto& element = elements[i];
    gin::Dictionary upload_data(isolate, v8::Object::New(isolate));
    switch (element.type()) {
      case network::mojom::DataElement::Tag::kFile: {
        const auto& element_file = element.As<network::DataElementFile>();
        upload_data.Set("type", "file");
        upload_data.Set("file", element_file.path().value());
        upload_data.Set("filePath",
                        base::Value(element_file.path().AsUTF8Unsafe()));
        upload_data.Set("offset", static_cast<int>(element_file.offset()));
        upload_data.Set("length", static_cast<int>(element_file.length()));
        upload_data.Set("modificationTime",
                        element_file.expected_modification_time().ToDoubleT());
        break;
      }
      case network::mojom::DataElement::Tag::kBytes: {
        upload_data.Set("type", "rawData");
        const auto& bytes = element.As<network::DataElementBytes>().bytes();
        const char* data = reinterpret_cast<const char*>(bytes.data());
        upload_data.Set(
            "bytes",
            node::Buffer::Copy(isolate, data, bytes.size()).ToLocalChecked());
        break;
      }
      case network::mojom::DataElement::Tag::kDataPipe: {
        upload_data.Set("type", "blob");
        // TODO(zcbenz): After the NetworkService refactor, the old blobUUID API
        // becomes unnecessarily complex, we should deprecate the getBlobData
        // API and return the DataPipeHolder wrapper directly.
        auto holder = electron::api::DataPipeHolder::Create(isolate, element);
        upload_data.Set("blobUUID", holder->id());
        // The lifetime of data pipe is bound to the uploadData object.
        upload_data.Set("dataPipe", holder);
        break;
      }
      default:
        NOTREACHED() << "Found unsupported data element";
    }
    arr->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i),
             ConvertToV8(isolate, upload_data))
        .Check();
  }
  return arr;
}

// static
v8::Local<v8::Value>
Converter<scoped_refptr<network::ResourceRequestBody>>::ToV8(
    v8::Isolate* isolate,
    const scoped_refptr<network::ResourceRequestBody>& val) {
  if (!val)
    return v8::Null(isolate);
  return ConvertToV8(isolate, *val);
}

// static
bool Converter<scoped_refptr<network::ResourceRequestBody>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    scoped_refptr<network::ResourceRequestBody>* out) {
  auto list = std::make_unique<base::ListValue>();
  if (!ConvertFromV8(isolate, val, list.get()))
    return false;
  *out = base::MakeRefCounted<network::ResourceRequestBody>();
  for (size_t i = 0; i < list->GetSize(); ++i) {
    base::DictionaryValue* dict = nullptr;
    std::string type;
    if (!list->GetDictionary(i, &dict))
      return false;
    dict->GetString("type", &type);
    if (type == "rawData") {
      base::Value* bytes = nullptr;
      dict->GetBinary("bytes", &bytes);
      (*out)->AppendBytes(
          reinterpret_cast<const char*>(bytes->GetBlob().data()),
          base::checked_cast<int>(bytes->GetBlob().size()));
    } else if (type == "file") {
      std::string file;
      int offset = 0, length = -1;
      double modification_time = 0.0;
      dict->GetStringWithoutPathExpansion("filePath", &file);
      dict->GetInteger("offset", &offset);
      dict->GetInteger("file", &length);
      dict->GetDouble("modificationTime", &modification_time);
      (*out)->AppendFileRange(base::FilePath::FromUTF8Unsafe(file),
                              static_cast<uint64_t>(offset),
                              static_cast<uint64_t>(length),
                              base::Time::FromDoubleT(modification_time));
    }
  }
  return true;
}

// static
v8::Local<v8::Value> Converter<network::ResourceRequest>::ToV8(
    v8::Isolate* isolate,
    const network::ResourceRequest& val) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("method", val.method);
  dict.Set("url", val.url.spec());
  dict.Set("referrer", val.referrer.spec());
  dict.Set("headers", val.headers);
  if (val.request_body)
    dict.Set("uploadData", ConvertToV8(isolate, *val.request_body));
  return ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<electron::VerifyRequestParams>::ToV8(
    v8::Isolate* isolate,
    electron::VerifyRequestParams val) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("hostname", val.hostname);
  dict.Set("certificate", val.certificate);
  dict.Set("validatedCertificate", val.validated_certificate);
  dict.Set("isIssuedByKnownRoot", val.is_issued_by_known_root);
  dict.Set("verificationResult", val.default_result);
  dict.Set("errorCode", val.error_code);
  return ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<net::HttpVersion>::ToV8(
    v8::Isolate* isolate,
    const net::HttpVersion& val) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  dict.Set("major", static_cast<uint32_t>(val.major_value()));
  dict.Set("minor", static_cast<uint32_t>(val.minor_value()));
  return ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<net::RedirectInfo>::ToV8(
    v8::Isolate* isolate,
    const net::RedirectInfo& val) {
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("statusCode", val.status_code);
  dict.Set("newMethod", val.new_method);
  dict.Set("newUrl", val.new_url);
  dict.Set("newSiteForCookies", val.new_site_for_cookies.RepresentativeUrl());
  dict.Set("newReferrer", val.new_referrer);
  dict.Set("insecureSchemeWasUpgraded", val.insecure_scheme_was_upgraded);
  dict.Set("isSignedExchangeFallbackRedirect",
           val.is_signed_exchange_fallback_redirect);

  return ConvertToV8(isolate, dict);
}

}  // namespace gin
