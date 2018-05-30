// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/promise_util.h"

namespace atom {

namespace util {

Promise* Promise::RejectWithErrorMessage(const std::string& string) {
  v8::Local<v8::String> error_message =
      v8::String::NewFromUtf8(isolate(), string.c_str());
  v8::Local<v8::Value> error = v8::Exception::Error(error_message);
  resolver_.Get(isolate())->Reject(mate::ConvertToV8(isolate(), error));
  return this;
}

v8::Local<v8::Object> Promise::GetHandle() const {
  return resolver_.Get(isolate())->GetPromise();
}

}  // namespace util

}  // namespace atom

namespace mate {

v8::Local<v8::Value> mate::Converter<atom::util::Promise*>::ToV8(
    v8::Isolate* isolate,
    atom::util::Promise* val) {
  return val->GetHandle();
}

}  // namespace mate
