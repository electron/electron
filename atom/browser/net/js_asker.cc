// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/js_asker.h"

#include <vector>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"

namespace atom {

namespace internal {

namespace {

// The callback which is passed to |handler|.
void HandlerCallback(const ResponseCallback& callback, mate::Arguments* args) {
  // If there is no argument passed then we failed.
  v8::Local<v8::Value> value;
  if (!args->GetNext(&value)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(callback, false, nullptr));
    return;
  }

  // Pass whatever user passed to the actaul request job.
  V8ValueConverter converter;
  v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
  scoped_ptr<base::Value> options(converter.FromV8Value(value, context));
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(callback, true, base::Passed(&options)));
}

}  // namespace

void AskForOptions(v8::Isolate* isolate,
                   const JavaScriptHandler& handler,
                   net::URLRequest* request,
                   const ResponseCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Context::Scope context_scope(context);
  handler.Run(request,
              mate::ConvertToV8(isolate,
                                base::Bind(&HandlerCallback, callback)));
}

bool IsErrorOptions(base::Value* value, int* error) {
  if (value->IsType(base::Value::TYPE_DICTIONARY)) {
    base::DictionaryValue* dict = static_cast<base::DictionaryValue*>(value);
    if (dict->GetInteger("error", error))
      return true;
  } else if (value->IsType(base::Value::TYPE_INTEGER)) {
    if (value->GetAsInteger(error))
      return true;
  }
  return false;
}

}  // namespace internal

}  // namespace atom
