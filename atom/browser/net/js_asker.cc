// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/js_asker.h"

#include <vector>

#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "native_mate/function_template.h"

namespace atom {

namespace internal {

namespace {

struct CallbackHolder {
  ResponseCallback callback;
};

// Cached JavaScript version of |HandlerCallback|.
v8::Persistent<v8::FunctionTemplate> g_handler_callback_;

// The callback which is passed to |handler|.
void HandlerCallback(v8::Isolate* isolate,
                     v8::Local<v8::External> external,
                     v8::Local<v8::Object> state,
                     mate::Arguments* args) {
  // Check if the callback has already been called.
  v8::Local<v8::String> called_symbol = mate::StringToSymbol(isolate, "called");
  if (state->Has(called_symbol))
    return;  // no nothing
  else
    state->Set(called_symbol, v8::Boolean::New(isolate, true));

  // If there is no argument passed then we failed.
  scoped_ptr<CallbackHolder> holder(
      static_cast<CallbackHolder*>(external->Value()));
  CHECK(holder);
  v8::Local<v8::Value> value;
  if (!args->GetNext(&value)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(holder->callback, false, nullptr));
    return;
  }

  // Pass whatever user passed to the actaul request job.
  V8ValueConverter converter;
  v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
  scoped_ptr<base::Value> options(converter.FromV8Value(value, context));
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(holder->callback, true, base::Passed(&options)));
}

// func.bind(func, arg1, arg2).
// NB(zcbenz): Using C++11 version crashes VS.
v8::Local<v8::Value> BindFunctionWith(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      v8::Local<v8::Function> func,
                                      v8::Local<v8::Value> arg1,
                                      v8::Local<v8::Value> arg2) {
  v8::MaybeLocal<v8::Value> bind = func->Get(mate::StringToV8(isolate, "bind"));
  CHECK(!bind.IsEmpty());
  v8::Local<v8::Function> bind_func =
      v8::Local<v8::Function>::Cast(bind.ToLocalChecked());
  v8::Local<v8::Value> converted[] = { func, arg1, arg2 };
  return bind_func->Call(
      context, func, arraysize(converted), converted).ToLocalChecked();
}

// Generate the callback that will be passed to |handler|.
v8::MaybeLocal<v8::Value> GenerateCallback(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context,
                                           const ResponseCallback& callback) {
  // The FunctionTemplate is cached.
  if (g_handler_callback_.IsEmpty())
    g_handler_callback_.Reset(
        isolate,
        mate::CreateFunctionTemplate(isolate, base::Bind(&HandlerCallback)));

  v8::Local<v8::FunctionTemplate> handler_callback =
      v8::Local<v8::FunctionTemplate>::New(isolate, g_handler_callback_);
  CallbackHolder* holder = new CallbackHolder;
  holder->callback = callback;
  return BindFunctionWith(isolate, context, handler_callback->GetFunction(),
                          v8::External::New(isolate, holder),
                          v8::Object::New(isolate));
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
  // We don't convert the callback to C++ directly because creating
  // FunctionTemplate will cause memory leak since V8 never releases it. So we
  // have to create the function object in JavaScript to work around it.
  v8::MaybeLocal<v8::Value> wrapped_callback = GenerateCallback(
      isolate, context, callback);
  if (wrapped_callback.IsEmpty()) {
    callback.Run(false, nullptr);
    return;
  }
  handler.Run(request, wrapped_callback.ToLocalChecked());
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
