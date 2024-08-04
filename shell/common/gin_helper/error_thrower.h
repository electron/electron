// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_ERROR_THROWER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_ERROR_THROWER_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "v8/include/v8-forward.h"

namespace gin_helper {

class ErrorThrower {
 public:
  explicit ErrorThrower(v8::Isolate* isolate);
  ErrorThrower();
  ~ErrorThrower() = default;

  void ThrowError(std::string_view err_msg) const;
  void ThrowTypeError(std::string_view err_msg) const;
  void ThrowRangeError(std::string_view err_msg) const;
  void ThrowReferenceError(std::string_view err_msg) const;
  void ThrowSyntaxError(std::string_view err_msg) const;

  v8::Isolate* isolate() const { return isolate_; }

 private:
  using ErrorGenerator = v8::Local<v8::Value> (*)(v8::Local<v8::String> err_msg,
                                                  v8::Local<v8::Value> options);
  void Throw(ErrorGenerator gen, std::string_view err_msg) const;

  raw_ptr<v8::Isolate> isolate_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_ERROR_THROWER_H_
