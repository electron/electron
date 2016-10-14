// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_

#include "base/memory/ref_counted.h"
#include "native_mate/converter.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace net {
class AuthChallengeInfo;
class URLRequest;
class X509Certificate;
class HttpResponseHeaders;
}

namespace mate {

template<>
struct Converter<const net::AuthChallengeInfo*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::AuthChallengeInfo* val);
};

template<>
struct Converter<scoped_refptr<net::X509Certificate>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const scoped_refptr<net::X509Certificate>& val);
};

template<>
struct Converter<scoped_refptr<net::HttpResponseHeaders>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      scoped_refptr<net::HttpResponseHeaders> headers);
};

}  // namespace mate

namespace atom {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request);

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request);

}  // namespace atom

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
