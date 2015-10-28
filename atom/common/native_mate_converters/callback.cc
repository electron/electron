// Copyright (c) 2015 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/callback.h"

namespace mate {

namespace internal {

namespace {

struct TranslaterHolder {
  Translater translater;
};

// Cached JavaScript version of |CallTranslater|.
v8::Persistent<v8::FunctionTemplate> g_call_translater;

void CallTranslater(v8::Local<v8::External> external, mate::Arguments* args) {
  TranslaterHolder* holder = static_cast<TranslaterHolder*>(external->Value());
  holder->translater.Run(args);
}

// func.bind(func, arg1).
// NB(zcbenz): Using C++11 version crashes VS.
v8::Local<v8::Value> BindFunctionWith(v8::Isolate* isolate,
                                      v8::Local<v8::Context> context,
                                      v8::Local<v8::Function> func,
                                      v8::Local<v8::Value> arg1) {
  v8::MaybeLocal<v8::Value> bind = func->Get(mate::StringToV8(isolate, "bind"));
  CHECK(!bind.IsEmpty());
  v8::Local<v8::Function> bind_func =
      v8::Local<v8::Function>::Cast(bind.ToLocalChecked());
  v8::Local<v8::Value> converted[] = { func, arg1 };
  return bind_func->Call(
      context, func, arraysize(converted), converted).ToLocalChecked();
}

}  // namespace

v8::Local<v8::Value> CreateFunctionFromTranslater(
    v8::Isolate* isolate, const Translater& translater) {
  // The FunctionTemplate is cached.
  if (g_call_translater.IsEmpty())
    g_call_translater.Reset(
        isolate,
        mate::CreateFunctionTemplate(isolate, base::Bind(&CallTranslater)));

  v8::Local<v8::FunctionTemplate> call_translater =
      v8::Local<v8::FunctionTemplate>::New(isolate, g_call_translater);
  TranslaterHolder* holder = new TranslaterHolder;
  holder->translater = translater;
  return BindFunctionWith(isolate,
                          isolate->GetCurrentContext(),
                          call_translater->GetFunction(),
                          v8::External::New(isolate, holder));
}

}  // namespace internal

}  // namespace mate
