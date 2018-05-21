// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "native_mate/promise.h"

namespace mate {

Promise::Promise()
    : isolate_(NULL) {
}

Promise::Promise(v8::Isolate* isolate)
    : isolate_(isolate) {
  resolver_ = v8::Promise::Resolver::New(isolate);
}

Promise::~Promise() {
}

Promise Promise::Create(v8::Isolate* isolate) {
  return Promise(isolate);
}

Promise Promise::Create() {
  return Promise::Create(v8::Isolate::GetCurrent());
}

void Promise::RejectWithErrorMessage(const std::string& string) {
  v8::Local<v8::String> error_message =
      v8::String::NewFromUtf8(isolate(), string.c_str());
  v8::Local<v8::Value> error = v8::Exception::Error(error_message);
  resolver_->Reject(mate::ConvertToV8(isolate(), error));
}

v8::Local<v8::Object> Promise::GetHandle() const {
  return resolver_->GetPromise();
}

v8::Local<v8::Value> Converter<Promise>::ToV8(v8::Isolate* isolate,
                                                  Promise val) {
  return val.GetHandle();
}

}  // namespace mate
