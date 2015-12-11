// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_

#include "atom/browser/net/atom_network_delegate.h"
#include "base/memory/ref_counted.h"
#include "native_mate/converter.h"

namespace net {
class AuthChallengeInfo;
class URLRequest;
class X509Certificate;
}

namespace mate {

template<>
struct Converter<const net::URLRequest*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::URLRequest* val);
};

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
struct Converter<atom::AtomNetworkDelegate::BlockingResponse> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::AtomNetworkDelegate::BlockingResponse* out);
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
