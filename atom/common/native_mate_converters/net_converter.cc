// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/net_converter.h"

#include <string>
#include <vector>

#include "atom/common/node_includes.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "native_mate/dictionary.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/upload_file_element_reader.h"
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
  const net::UploadDataStream* upload_data = val->get_upload();
  if (upload_data) {
    const ScopedVector<net::UploadElementReader>* readers =
        upload_data->GetElementReaders();
    std::vector<mate::Dictionary> upload_data_list;
    upload_data_list.reserve(readers->size());
    for (const auto& reader : *readers) {
      auto upload_data_dict = mate::Dictionary::CreateEmpty(isolate);
      if (reader->AsBytesReader()) {
        const net::UploadBytesElementReader* bytes_reader =
            reader->AsBytesReader();
        auto bytes =
            node::Buffer::Copy(isolate, bytes_reader->bytes(),
                               bytes_reader->length()).ToLocalChecked();
        upload_data_dict.Set("bytes", bytes);
      } else if (reader->AsFileReader()) {
        const net::UploadFileElementReader* file_reader =
            reader->AsFileReader();
        upload_data_dict.Set("file", file_reader->path().AsUTF8Unsafe());
      }
      upload_data_list.push_back(upload_data_dict);
    }
    dict.Set("uploadData", upload_data_list);
  }
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

}  // namespace mate
