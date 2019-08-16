// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_ERROR_UTIL_H_
#define SHELL_COMMON_ERROR_UTIL_H_

#include <string>
#include <utility>

#include "native_mate/converter.h"
#include "third_party/blink/renderer/platform/bindings/v8_throw_exception.h"

namespace electron {

namespace util {

// A wrapper around V8ThrowException.

class Error {
 public:
  Error(v8::Isolate* isolate);
  Error();

  ~Error();

  v8::Isolate* isolate() const { return isolate_; }

  void ThrowError(const std::string& err_msg);
  void ThrowTypeError(const std::string& err_msg);
  void ThrowRangeError(const std::string& err_msg);
  void ThrowReferenceError(const std::string& err_msg);
  void ThrowSyntaxError(const std::string& err_msg);

 private:
  v8::Isolate* isolate_;
};

}  // namespace util

}  // namespace electron

#endif  // SHELL_COMMON_ERROR_UTIL_H_
