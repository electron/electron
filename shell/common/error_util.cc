// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "native_mate/converter.h"
#include "shell/common/error_util.h"

namespace electron {

namespace util {

ErrorThrower::ErrorThrower(v8::Isolate* isolate) : isolate_(isolate) {}

// This constructor should be rarely if ever used, since
// v8::Isolate::GetCurrent() uses atomic loads and is thus a bit
// costly to invoke
ErrorThrower::ErrorThrower() : isolate_(v8::Isolate::GetCurrent()) {}

ErrorThrower::~ErrorThrower() = default;

void ErrorThrower::ThrowError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::Error(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void ErrorThrower::ThrowTypeError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::TypeError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void ErrorThrower::ThrowRangeError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::RangeError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void ErrorThrower::ThrowReferenceError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::ReferenceError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void ErrorThrower::ThrowSyntaxError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::SyntaxError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

}  // namespace util

}  // namespace electron
