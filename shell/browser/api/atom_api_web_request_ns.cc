// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_web_request_ns.h"

#include <set>
#include <string>

#include "extensions/common/url_pattern.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/gin_converters/callback_converter_gin_adapter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter_gin_adapter.h"

namespace gin {

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

}  // namespace gin

namespace electron {

namespace api {

namespace {

const char* kUserDataKey = "WebRequestNS";

struct UserData : public base::SupportsUserData::Data {
  explicit UserData(WebRequestNS* data) : data(data) {}
  WebRequestNS* data;
};

}  // namespace

gin::WrapperInfo WebRequestNS::kWrapperInfo = {gin::kEmbedderNativeGin};

WebRequestNS::WebRequestNS(v8::Isolate* isolate,
                           content::BrowserContext* browser_context) {
  browser_context->SetUserData(kUserDataKey, std::make_unique<UserData>(this));
}

WebRequestNS::~WebRequestNS() = default;

gin::ObjectTemplateBuilder WebRequestNS::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebRequestNS>::GetObjectTemplateBuilder(isolate)
      .SetMethod("onBeforeRequest",
                 &WebRequestNS::SetResponseListener<kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequestNS::SetResponseListener<kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequestNS::SetResponseListener<kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequestNS::SetSimpleListener<kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequestNS::SetSimpleListener<kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequestNS::SetSimpleListener<kOnResponseStarted>)
      .SetMethod("onErrorOccurred",
                 &WebRequestNS::SetSimpleListener<kOnErrorOccurred>)
      .SetMethod("onCompleted", &WebRequestNS::SetSimpleListener<kOnCompleted>);
}

const char* WebRequestNS::GetTypeName() {
  return "WebRequest";
}

int WebRequestNS::OnBeforeRequest(net::CompletionOnceCallback callback,
                                  GURL* new_url) {
  return net::OK;
}

int WebRequestNS::OnBeforeSendHeaders(BeforeSendHeadersCallback callback,
                                      net::HttpRequestHeaders* headers) {
  return net::OK;
}

int WebRequestNS::OnHeadersReceived(
    net::CompletionOnceCallback callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return net::OK;
}

void WebRequestNS::OnSendHeaders(const net::HttpRequestHeaders& headers) {}

void WebRequestNS::OnBeforeRedirect(const GURL& new_location) {}

void WebRequestNS::OnResponseStarted(int net_error) {}

void WebRequestNS::OnErrorOccurred(int net_error) {}

void WebRequestNS::OnCompleted(int net_error) {}

void WebRequestNS::OnResponseStarted() {}

template <WebRequestNS::SimpleEvent event>
void WebRequestNS::SetSimpleListener(gin::Arguments* args) {
  SetListener<SimpleListener, SimpleEvent>(event, args);
}

template <WebRequestNS::ResponseEvent event>
void WebRequestNS::SetResponseListener(gin::Arguments* args) {
  SetListener<ResponseListener, ResponseEvent>(event, args);
}

template <typename Listener, typename Event>
void WebRequestNS::SetListener(Event event, gin::Arguments* args) {
  // { urls }.
  std::set<URLPattern> patterns;
  gin::Dictionary dict(args->isolate());
  args->GetNext(&dict) && dict.Get("urls", &patterns);

  // Function or null.
  v8::Local<v8::Value> value;
  Listener listener;
  if (!args->GetNext(&listener) &&
      !(args->GetNext(&value) && value->IsNull())) {
    args->ThrowTypeError("Must pass null or a Function");
    return;
  }

  // TODO(zcbenz): Actually set the listeners.
  args->ThrowTypeError("This API is not implemented yet");
}

// static
gin::Handle<WebRequestNS> WebRequestNS::FromOrCreate(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  auto* web_request = From(browser_context);
  if (!web_request)
    web_request = new WebRequestNS(isolate, browser_context);
  return gin::CreateHandle(isolate, web_request);
}

// static
WebRequestNS* WebRequestNS::From(content::BrowserContext* browser_context) {
  if (!browser_context)
    return nullptr;
  auto* user_data =
      static_cast<UserData*>(browser_context->GetUserData(kUserDataKey));
  if (!user_data)
    return nullptr;
  return user_data->data;
}

}  // namespace api

}  // namespace electron
