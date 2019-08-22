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
  Throw(v8::Exception::Error, err_msg);
}

void ErrorThrower::ThrowTypeError(const std::string& err_msg) {
  Throw(v8::Exception::TypeError, err_msg);
}

void ErrorThrower::ThrowRangeError(const std::string& err_msg) {
  Throw(v8::Exception::RangeError, err_msg);
}

void ErrorThrower::ThrowReferenceError(const std::string& err_msg) {
  Throw(v8::Exception::ReferenceError, err_msg);
}

void ErrorThrower::ThrowSyntaxError(const std::string& err_msg) {
  Throw(v8::Exception::SyntaxError, err_msg);
}

}  // namespace util

}  // namespace electron
