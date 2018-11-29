// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/js_asker.h"

#include <utility>

#include "atom/common/native_mate_converters/callback.h"
#include "content/public/browser/browser_thread.h"

namespace atom {

JsAsker::JsAsker() = default;

JsAsker::~JsAsker() = default;

void JsAsker::SetHandlerInfo(
    v8::Isolate* isolate,
    net::URLRequestContextGetter* request_context_getter,
    const JavaScriptHandler& handler) {
  isolate_ = isolate;
  request_context_getter_ = request_context_getter;
  handler_ = handler;
}

// static
void JsAsker::AskForOptions(
    v8::Isolate* isolate,
    const JavaScriptHandler& handler,
    std::unique_ptr<base::DictionaryValue> request_details,
    const BeforeStartCallback& before_start) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Context::Scope context_scope(context);
  handler.Run(*(request_details.get()),
              mate::ConvertToV8(isolate, before_start));
}

// static
bool JsAsker::IsErrorOptions(base::Value* value, int* error) {
  if (value->is_dict()) {
    base::DictionaryValue* dict = static_cast<base::DictionaryValue*>(value);
    if (dict->GetInteger("error", error))
      return true;
  } else if (value->is_int()) {
    *error = value->GetInt();
    return true;
  }
  return false;
}

}  // namespace atom
