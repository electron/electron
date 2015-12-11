// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_request.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

using content::BrowserThread;

namespace atom {

namespace api {

WebRequest::WebRequest(AtomBrowserContext* browser_context)
    : browser_context_(browser_context) {
}

WebRequest::~WebRequest() {
}

template<AtomNetworkDelegate::EventTypes type>
void WebRequest::SetListener(mate::Arguments* args) {
  scoped_ptr<base::DictionaryValue> filter(new base::DictionaryValue);
  args->GetNext(filter.get());

  v8::Local<v8::Value> value;
  AtomNetworkDelegate::Listener callback;
  if (!args->GetNext(&callback) &&
      !(args->GetNext(&value) && value->IsNull())) {
    args->ThrowError("Must pass null or a Function");
    return;
  }

  auto delegate = browser_context_->network_delegate();
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::Bind(&AtomNetworkDelegate::SetListenerInIO,
                                     base::Unretained(delegate),
                                     type, base::Passed(&filter), callback));
}

// static
mate::Handle<WebRequest> WebRequest::Create(
    v8::Isolate* isolate,
    AtomBrowserContext* browser_context) {
  return mate::CreateHandle(isolate, new WebRequest(browser_context));
}

// static
void WebRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("onBeforeRequest",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnBeforeSendHeaders>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnHeadersReceived>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnResponseStarted>)
      .SetMethod("onCompleted",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnCompleted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetListener<
                    AtomNetworkDelegate::kOnErrorOccurred>);
}

}  // namespace api

}  // namespace atom
