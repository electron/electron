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

  v8::Isolate* isolate_;
};

}  // namespace util

}  // namespace electron

#endif  // SHELL_COMMON_ERROR_UTIL_H_
