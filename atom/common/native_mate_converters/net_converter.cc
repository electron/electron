// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/net_converter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "native_mate/dictionary.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "storage/browser/blob/upload_blob_element_reader.h"

#include "atom/common/node_includes.h"

namespace mate {

namespace {

bool CertFromData(const std::string& data,
                  scoped_refptr<net::X509Certificate>* out) {
  auto cert_list = net::X509Certificate::CreateCertificateListFromBytes(
      data.c_str(), data.length(),
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
v8::Local<v8::Value> Converter<const net::AuthChallengeInfo*>::ToV8(
    v8::Isolate* isolate,
    const net::AuthChallengeInfo* val) {
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
    v8::Isolate* isolate,
    const scoped_refptr<net::X509Certificate>& val) {
  mate::Dictionary dict(isolate, v8::Object::New(isolate));
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

  return dict.GetHandle();
}

bool Converter<scoped_refptr<net::X509Certificate>>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    scoped_refptr<net::X509Certificate>* out) {
  mate::Dictionary dict;
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
  mate::Dictionary dict(isolate, v8::Object::New(isolate));

  dict.Set("commonName", val.common_name);
  dict.Set("organizations", val.organization_names);
  dict.Set("organizationUnits", val.organization_unit_names);
  dict.Set("locality", val.locality_name);
  dict.Set("state", val.state_or_province_name);
  dict.Set("country", val.country_name);

  return dict.GetHandle();
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
      if (response_headers.FindKey(key)) {
        base::ListValue* values = nullptr;
        if (response_headers.GetList(key, &values))
          values->AppendString(value);
      } else {
        auto values = std::make_unique<base::ListValue>();
        values->AppendString(value);
        response_headers.Set(key, std::move(values));
      }
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
  auto context = isolate->GetCurrentContext();
  auto headers = v8::Local<v8::Object>::Cast(val);
  auto keys = headers->GetOwnPropertyNames();
  for (uint32_t i = 0; i < keys->Length(); i++) {
    v8::Local<v8::String> key, value;
    if (!keys->Get(i)->ToString(context).ToLocal(&key)) {
      return false;
    }
    if (!headers->Get(key)->ToString(context).ToLocal(&value)) {
      return false;
    }
    v8::String::Utf8Value key_utf8(key);
    v8::String::Utf8Value value_utf8(value);
    std::string k(*key_utf8, key_utf8.length());
    std::string v(*value_utf8, value_utf8.length());
    std::ostringstream tmp;
    tmp << k << ": " << v;
    out->AddHeader(tmp.str());
  }
  return true;
}

}  // namespace mate

namespace atom {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request) {
  details->SetString("method", request->method());
  std::string url;
  if (!request->url_chain().empty())
    url = request->url().spec();
  details->SetKey("url", base::Value(url));
  details->SetString("referrer", request->referrer());
  auto list = std::make_unique<base::ListValue>();
  GetUploadData(list.get(), request);
  if (!list->empty())
    details->Set("uploadData", std::move(list));
  auto headers_value = std::make_unique<base::DictionaryValue>();
  for (net::HttpRequestHeaders::Iterator it(request->extra_request_headers());
       it.GetNext();) {
    headers_value->SetString(it.name(), it.value());
  }
  details->Set("headers", std::move(headers_value));
}

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request) {
  const net::UploadDataStream* upload_data = request->get_upload();
  if (!upload_data)
    return;
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      upload_data->GetElementReaders();
  for (const auto& reader : *readers) {
    auto upload_data_dict = std::make_unique<base::DictionaryValue>();
    if (reader->AsBytesReader()) {
      const net::UploadBytesElementReader* bytes_reader =
          reader->AsBytesReader();
      auto bytes = std::make_unique<base::Value>(
          std::vector<char>(bytes_reader->bytes(),
                            bytes_reader->bytes() + bytes_reader->length()));
      upload_data_dict->Set("bytes", std::move(bytes));
    } else if (reader->AsFileReader()) {
      const net::UploadFileElementReader* file_reader = reader->AsFileReader();
      auto file_path = file_reader->path().AsUTF8Unsafe();
      upload_data_dict->SetKey("file", base::Value(file_path));
    } else {
      const storage::UploadBlobElementReader* blob_reader =
          static_cast<storage::UploadBlobElementReader*>(reader.get());
      upload_data_dict->SetString("blobUUID", blob_reader->uuid());
    }
    upload_data_list->Append(std::move(upload_data_dict));
  }
}

}  // namespace atom
