// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string_view>

#include "shell/common/gin_helper/error_thrower.h"

#include "gin/converter.h"

namespace gin_helper {

v8::Isolate* ErrorThrower::isolate() const {
  // Callers should prefer to specify the isolate in the constructor,
  // since GetCurrent() uses atomic loads and is thus a bit costly to invoke
  return isolate_ ? isolate_.get() : v8::Isolate::GetCurrent();
}

void ErrorThrower::ThrowError(const std::string_view err_msg) const {
  Throw(v8::Exception::Error, err_msg);
}

void ErrorThrower::ThrowTypeError(const std::string_view err_msg) const {
  Throw(v8::Exception::TypeError, err_msg);
}

void ErrorThrower::ThrowRangeError(const std::string_view err_msg) const {
  Throw(v8::Exception::RangeError, err_msg);
}

void ErrorThrower::ThrowReferenceError(const std::string_view err_msg) const {
  Throw(v8::Exception::ReferenceError, err_msg);
}

void ErrorThrower::ThrowSyntaxError(const std::string_view err_msg) const {
  Throw(v8::Exception::SyntaxError, err_msg);
}

void ErrorThrower::Throw(ErrorGenerator gen,
                         const std::string_view err_msg) const {
  v8::Isolate* isolate = this->isolate();

  if (!isolate->IsExecutionTerminating())
    isolate->ThrowException(gen(gin::StringToV8(isolate, err_msg), {}));
}

}  // namespace gin_helper
