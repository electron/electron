// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/arguments.h"

#include "base/strings/stringprintf.h"

namespace gin_helper {

namespace {

// Try to get a readable type of |value|.
std::string V8TypeAsString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (value.IsEmpty())
    return "<empty handle>";
  if (value->IsUndefined())
    return "undefined";
  if (value->IsNull())
    return "null";
  std::string result;
  if (value->IsObject()) {
    if (gin::ConvertFromV8(
            isolate, value.As<v8::Object>()->GetConstructorName(), &result))
      return result;
  }
  if (gin::ConvertFromV8(isolate, value, &result))
    return '"' + result + '"';
  if (gin::ConvertFromV8(isolate, value->TypeOf(isolate), &result))
    return result;
  return "<unkown>";
}

}  // namespace

void Arguments::ThrowError() const {
  if (is_for_property_ || insufficient_arguments_) {
    gin::Arguments::ThrowError();
    return;
  }
  // The |gin::Arguments| class advances |next_| counter when conversion fails
  // while |gin_helper::Arguments| does not, so we use |next_| instead of
  // |next_ - 1| as current argument index.
  return ThrowTypeError(base::StringPrintf(
      "Error processing argument at index %d, conversion failure from %s",
      next_, V8TypeAsString(isolate_, (*info_for_function_)[next_]).c_str()));
}

void Arguments::ThrowError(base::StringPiece message) const {
  isolate()->ThrowException(
      v8::Exception::Error(gin::StringToV8(isolate(), message)));
}

}  // namespace gin_helper
