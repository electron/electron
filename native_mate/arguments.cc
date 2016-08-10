// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/arguments.h"

#include "base/strings/stringprintf.h"
#include "native_mate/converter.h"

namespace mate {

namespace {

std::string V8TypeAsString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  v8::MaybeLocal<v8::String> details = value->ToDetailString(isolate);
  std::string result;
  if (!details.IsEmpty())
    ConvertFromV8(isolate, details.ToLocalChecked(), &result);
  return result;
}

}  // namespace

Arguments::Arguments()
    : isolate_(NULL),
      info_(NULL),
      next_(0),
      insufficient_arguments_(false) {
}

Arguments::Arguments(const MATE_METHOD_ARGS_TYPE& info)
    : isolate_(info.GetIsolate()),
      info_(&info),
      next_(0),
      insufficient_arguments_(false) {
}

Arguments::~Arguments() {
}

v8::Local<v8::Value> Arguments::PeekNext() const {
  if (next_ >= info_->Length())
    return v8::Local<v8::Value>();
  return (*info_)[next_];
}

v8::Local<v8::Value> Arguments::ThrowError() const {
  if (insufficient_arguments_)
    return ThrowTypeError("Insufficient number of arguments.");

  return ThrowTypeError(base::StringPrintf(
      "Error processing argument at index %d, conversion failure from %s",
      next_, V8TypeAsString(isolate_, (*info_)[next_]).c_str()));
}

v8::Local<v8::Value> Arguments::ThrowError(const std::string& message) const {
  MATE_THROW_EXCEPTION(isolate_, v8::Exception::Error(
      StringToV8(isolate_, message)));
  return MATE_UNDEFINED(isolate_);
}

v8::Local<v8::Value> Arguments::ThrowTypeError(
    const std::string& message) const {
  MATE_THROW_EXCEPTION(isolate_, v8::Exception::TypeError(
      StringToV8(isolate_, message)));
  return MATE_UNDEFINED(isolate_);
}

}  // namespace mate
