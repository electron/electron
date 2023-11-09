// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/error_thrower.h"

#include "gin/converter.h"

namespace gin_helper {

ErrorThrower::ErrorThrower(v8::Isolate* isolate) : isolate_(isolate) {}

// This constructor should be rarely if ever used, since
// v8::Isolate::GetCurrent() uses atomic loads and is thus a bit
// costly to invoke
ErrorThrower::ErrorThrower() : isolate_(v8::Isolate::GetCurrent()) {}

void ErrorThrower::ThrowError(base::StringPiece err_msg) const {
  Throw(v8::Exception::Error, err_msg);
}

void ErrorThrower::ThrowTypeError(base::StringPiece err_msg) const {
  Throw(v8::Exception::TypeError, err_msg);
}

void ErrorThrower::ThrowRangeError(base::StringPiece err_msg) const {
  Throw(v8::Exception::RangeError, err_msg);
}

void ErrorThrower::ThrowReferenceError(base::StringPiece err_msg) const {
  Throw(v8::Exception::ReferenceError, err_msg);
}

void ErrorThrower::ThrowSyntaxError(base::StringPiece err_msg) const {
  Throw(v8::Exception::SyntaxError, err_msg);
}

void ErrorThrower::Throw(ErrorGenerator gen, base::StringPiece err_msg) const {
  v8::Local<v8::Value> exception = gen(gin::StringToV8(isolate_, err_msg), {});
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

}  // namespace gin_helper
