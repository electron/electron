// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/arguments.h"

#include "base/strings/stringprintf.h"
#include "native_mate/converter.h"

namespace mate {

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

v8::Handle<v8::Value> Arguments::PeekNext() const {
  if (next_ >= info_->Length())
    return v8::Handle<v8::Value>();
  return (*info_)[next_];
}

void Arguments::ThrowError() const {
  if (insufficient_arguments_)
    return ThrowTypeError("Insufficient number of arguments.");

  ThrowTypeError(base::StringPrintf(
      "Error processing argument %d.", next_ - 1));
}

void Arguments::ThrowTypeError(const std::string& message) const {
  MATE_THROW_EXCEPTION(isolate_, v8::Exception::TypeError(
      StringToV8(isolate_, message)));
}

}  // namespace mate
