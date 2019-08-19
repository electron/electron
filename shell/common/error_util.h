// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_ERROR_UTIL_H_
#define SHELL_COMMON_ERROR_UTIL_H_

#include <string>

#include "native_mate/converter.h"

namespace electron {

namespace util {

class ErrorThrower {
 public:
  explicit ErrorThrower(v8::Isolate* isolate);
  ErrorThrower();

  ~ErrorThrower();

  void ThrowError(const std::string& err_msg);
  void ThrowTypeError(const std::string& err_msg);
  void ThrowRangeError(const std::string& err_msg);
  void ThrowReferenceError(const std::string& err_msg);
  void ThrowSyntaxError(const std::string& err_msg);

 private:
  v8::Isolate* isolate() const { return isolate_; }

  using ErrorGenerator =
      v8::Local<v8::Value> (*)(v8::Local<v8::String> err_msg);
  void Throw(ErrorGenerator gen, const std::string& err_msg) {
    v8::Local<v8::Value> exception = gen(mate::StringToV8(isolate_, err_msg));
    if (!isolate_->IsExecutionTerminating())
      isolate_->ThrowException(exception);
  }

  v8::Isolate* isolate_;
};

}  // namespace util

}  // namespace electron

#endif  // SHELL_COMMON_ERROR_UTIL_H_
