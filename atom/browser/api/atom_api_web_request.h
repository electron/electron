// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_

#include <map>
#include <string>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_util.h"
#include "native_mate/arguments.h"
#include "native_mate/handle.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace mate {

template<>
struct Converter<net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   net::HttpResponseHeaders* headers) {
    base::DictionaryValue response_headers;
    if (headers) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        if (response_headers.HasKey(key)) {
          base::ListValue* values = nullptr;
          if (response_headers.GetList(key, &values))
            values->AppendString(value);
        } else {
          std::unique_ptr<base::ListValue> values(new base::ListValue());
          values->AppendString(value);
          response_headers.Set(key, std::move(values));
        }
      }
    }
    return ConvertToV8(isolate, response_headers);
  }
};

template<>
struct Converter<net::HttpRequestHeaders> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     net::HttpRequestHeaders* out) {
    net::HttpRequestHeaders request_headers;
    *out = request_headers;
    base::DictionaryValue headers;
    if (!ConvertFromV8(isolate, val, &headers))
      return false;

    for (base::DictionaryValue::Iterator it(headers); !it.IsAtEnd();
         it.Advance()) {
      std::string value;
      if (!it.value().GetAsString(&value))
        continue;

      out->SetHeader(it.key(), value);
    }
    return true;
  }
};

class Dictionary;

}  // namespace mate

namespace atom {

class AtomBrowserContext;

namespace api {

class WebRequest : public mate::TrackableObject<WebRequest>,
                   public net::URLFetcherDelegate {
 public:
  static mate::Handle<WebRequest> Create(v8::Isolate* isolate,
                                         AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  WebRequest(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~WebRequest() override;

  typedef base::Callback<void(
      v8::Local<v8::Value>, mate::Dictionary, std::string)> FetchCallback;
  void Fetch(mate::Arguments* args);
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // C++ can not distinguish overloaded member function.
  template<AtomNetworkDelegate::SimpleEvent type>
  void SetSimpleListener(mate::Arguments* args);
  template<AtomNetworkDelegate::ResponseEvent type>
  void SetResponseListener(mate::Arguments* args);
  template<typename Listener, typename Method, typename Event>
  void SetListener(Method method, Event type, mate::Arguments* args);

 private:
  scoped_refptr<AtomBrowserContext> browser_context_;
  std::map<const net::URLFetcher*, FetchCallback> fetchers_;

  DISALLOW_COPY_AND_ASSIGN(WebRequest);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_REQUEST_H_
