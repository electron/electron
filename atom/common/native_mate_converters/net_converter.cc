// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/net_converter.h"

#include "native_mate/dictionary.h"
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

}  // namespace mate
