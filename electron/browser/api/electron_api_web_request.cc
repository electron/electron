// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/browser/api/electron_api_web_request.h"

#include <string>

#include "electron/browser/electron_browser_context.h"
#include "electron/browser/net/electron_network_delegate.h"
#include "electron/common/native_mate_converters/callback.h"
#include "electron/common/native_mate_converters/net_converter.h"
#include "electron/common/native_mate_converters/value_converter.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

using content::BrowserThread;

namespace mate {

template<>
struct Converter<extensions::URLPattern> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     extensions::URLPattern* out) {
    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;
    return out->Parse(pattern) == extensions::URLPattern::PARSE_SUCCESS;
  }
};

}  // namespace mate

namespace electron {

namespace api {

WebRequest::WebRequest(ElectronBrowserContext* browser_context)
    : browser_context_(browser_context) {
}

WebRequest::~WebRequest() {
}

template<ElectronNetworkDelegate::SimpleEvent type>
void WebRequest::SetSimpleListener(mate::Arguments* args) {
  SetListener<ElectronNetworkDelegate::SimpleListener>(
      &ElectronNetworkDelegate::SetSimpleListenerInIO, type, args);
}

template<ElectronNetworkDelegate::ResponseEvent type>
void WebRequest::SetResponseListener(mate::Arguments* args) {
  SetListener<ElectronNetworkDelegate::ResponseListener>(
      &ElectronNetworkDelegate::SetResponseListenerInIO, type, args);
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
    ElectronBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new WebRequest(browser_context));
}

// static
void WebRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("onBeforeRequest",
                 &WebRequest::SetResponseListener<
                    ElectronNetworkDelegate::kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetResponseListener<
                    ElectronNetworkDelegate::kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetResponseListener<
                    ElectronNetworkDelegate::kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetSimpleListener<
                    ElectronNetworkDelegate::kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<
                    ElectronNetworkDelegate::kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetSimpleListener<
                    ElectronNetworkDelegate::kOnResponseStarted>)
      .SetMethod("onCompleted",
                 &WebRequest::SetSimpleListener<
                    ElectronNetworkDelegate::kOnCompleted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetSimpleListener<
                    ElectronNetworkDelegate::kOnErrorOccurred>);
}

}  // namespace api

}  // namespace electron
