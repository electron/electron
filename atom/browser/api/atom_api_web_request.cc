// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_request.h"

#include <string>
#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

using content::BrowserThread;

namespace mate {

template <>
struct Converter<URLPattern> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     URLPattern* out) {
    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;
    *out = URLPattern(URLPattern::SCHEME_ALL);
    return out->Parse(pattern) == URLPattern::ParseResult::kSuccess;
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

template <typename Method, typename Event, typename Listener>
void CallNetworkDelegateMethod(
    URLRequestContextGetter* url_request_context_getter,
    Method method,
    Event type,
    URLPatterns patterns,
    Listener listener) {
  // Force creating network delegate.
  url_request_context_getter->GetURLRequestContext();
  // Then call the method.
  auto* network_delegate = url_request_context_getter->network_delegate();
  (network_delegate->*method)(type, std::move(patterns), std::move(listener));
}

}  // namespace

WebRequest::WebRequest(v8::Isolate* isolate,
                       AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
  Init(isolate);
}

WebRequest::~WebRequest() {}

template <AtomNetworkDelegate::SimpleEvent type>
void WebRequest::SetSimpleListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::SimpleListener>(
      &AtomNetworkDelegate::SetSimpleListenerInIO, type, args);
}

template <AtomNetworkDelegate::ResponseEvent type>
void WebRequest::SetResponseListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::ResponseListener>(
      &AtomNetworkDelegate::SetResponseListenerInIO, type, args);
}

template <typename Listener, typename Method, typename Event>
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

  auto* url_request_context_getter = static_cast<URLRequestContextGetter*>(
      browser_context_->GetRequestContext());
  if (!url_request_context_getter)
    return;
  base::PostTaskWithTraits(
      FROM_HERE, {BrowserThread::IO},
      base::BindOnce(&CallNetworkDelegateMethod<Method, Event, Listener>,
                     base::RetainedRef(url_request_context_getter), method,
                     type, std::move(patterns), std::move(listener)));
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
      .SetMethod("onBeforeRequest", &WebRequest::SetResponseListener<
                                        AtomNetworkDelegate::kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetResponseListener<
                     AtomNetworkDelegate::kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetResponseListener<
                     AtomNetworkDelegate::kOnHeadersReceived>)
      .SetMethod(
          "onSendHeaders",
          &WebRequest::SetSimpleListener<AtomNetworkDelegate::kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<
                     AtomNetworkDelegate::kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetSimpleListener<
                     AtomNetworkDelegate::kOnResponseStarted>)
      .SetMethod(
          "onCompleted",
          &WebRequest::SetSimpleListener<AtomNetworkDelegate::kOnCompleted>)
      .SetMethod("onErrorOccurred", &WebRequest::SetSimpleListener<
                                        AtomNetworkDelegate::kOnErrorOccurred>);
}

}  // namespace api

}  // namespace atom
