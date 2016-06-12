// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

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

}  // namespace mate

namespace atom {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request);

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request);

}  // namespace atom
