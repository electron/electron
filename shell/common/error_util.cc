// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "native_mate/converter.h"
#include "shell/common/error_util.h"

namespace electron {

namespace util {

Error::Error(v8::Isolate* isolate) : isolate_(isolate) {}

// This constructor should be rarely if ever used, since
// v8::Isolate::GetCurrent() uses atomic loads and is thus a bit
// costly to invoke
Error::Error() : isolate_(v8::Isolate::GetCurrent()) {}

Error::~Error() = default;

void Error::ThrowError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::Error(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void Error::ThrowTypeError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::TypeError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void Error::ThrowRangeError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::RangeError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void Error::ThrowReferenceError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::ReferenceError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

void Error::ThrowSyntaxError(const std::string& err_msg) {
  v8::Local<v8::Value> exception =
      v8::Exception::SyntaxError(mate::StringToV8(isolate_, err_msg));
  if (!isolate_->IsExecutionTerminating())
    isolate_->ThrowException(exception);
}

}  // namespace util

}  // namespace electron
