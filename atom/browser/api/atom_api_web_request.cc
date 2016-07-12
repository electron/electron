// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_request.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/files/file_path.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

using content::BrowserThread;

namespace mate {

template<>
struct Converter<URLPattern> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     URLPattern* out) {
    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;
    return out->Parse(pattern) == URLPattern::PARSE_SUCCESS;
  }
};

template<>
struct Converter<net::URLFetcher::RequestType> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     net::URLFetcher::RequestType* out) {
    std::string type = base::ToLowerASCII(V8ToString(val));
    if (type == "get")
      *out = net::URLFetcher::RequestType::GET;
    else if (type == "post")
      *out = net::URLFetcher::RequestType::POST;
    else if (type == "head")
      *out = net::URLFetcher::RequestType::HEAD;
    else if (type == "delete_request")
      *out = net::URLFetcher::RequestType::DELETE_REQUEST;
    else if (type == "put")
      *out = net::URLFetcher::RequestType::PUT;
    else if (type == "patch")
      *out = net::URLFetcher::RequestType::PATCH;
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

WebRequest::WebRequest(v8::Isolate* isolate,
                       AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  Init(isolate);
}

WebRequest::~WebRequest() {
}

void WebRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  std::string response;
  source->GetResponseAsString(&response);
  int response_code = source->GetResponseCode();
  net::HttpResponseHeaders* headers = source->GetResponseHeaders();
  FetchCallback callback = fetchers_[source];
  callback.Run(response_code, response, headers);
  fetchers_.erase(source);
}

void WebRequest::Fetch(mate::Arguments* args) {
  GURL url;
  if (!args->GetNext(&url)) {
    args->ThrowError();
    return;
  }

  net::URLFetcher::RequestType request_type;
  if (!args->GetNext(&request_type)) {
    args->ThrowError();
    return;
  }

  net::HttpRequestHeaders* headers = nullptr;
  if (!args->GetNext(&headers)) {
    args->ThrowError();
    return;
  }

  base::FilePath path;
  if (!args->GetNext(&path)) {
    // ignore optional argument
  }

  FetchCallback callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }

  net::URLFetcher* fetcher = net::URLFetcher::Create(url, request_type, this)
      .release();
  fetcher->SetRequestContext(browser_context_->GetRequestContext());
  fetcher->SetExtraRequestHeaders(headers->ToString());
  if (!path.empty())
    fetcher->SaveResponseToFileAtPath(
        path,
        BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE));
  fetcher->Start();
  fetchers_[fetcher] = FetchCallback(callback);
}

template<AtomNetworkDelegate::SimpleEvent type>
void WebRequest::SetSimpleListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::SimpleListener>(
      &AtomNetworkDelegate::SetSimpleListenerInIO, type, args);
}

template<AtomNetworkDelegate::ResponseEvent type>
void WebRequest::SetResponseListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::ResponseListener>(
      &AtomNetworkDelegate::SetResponseListenerInIO, type, args);
}

template<typename Listener, typename Method, typename Event>
void WebRequest::SetListener(Method method, Event type, mate::Arguments* args) {
  // { urls }.
  URLPatterns patterns;
  mate::Dictionary dict;
  args->GetNext(&dict) && dict.Get("urls", &patterns);

  // Function or null.
  v8::Local<v8::Value> value;
  Listener listener;
  if (!args->GetNext(&listener) &&
      !(args->GetNext(&value) && value->IsNull())) {
    args->ThrowError("Must pass null or a Function");
    return;
  }

  auto delegate = browser_context_->network_delegate();
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(method, base::Unretained(delegate), type,
                                     patterns, listener));
}

// static
mate::Handle<WebRequest> WebRequest::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new WebRequest(isolate, browser_context));
}

// static
void WebRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("onBeforeRequest",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnResponseStarted>)
      .SetMethod("onCompleted",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnCompleted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnErrorOccurred>)
      .SetMethod("fetch",
                 &WebRequest::Fetch);
}

}  // namespace api

}  // namespace atom
