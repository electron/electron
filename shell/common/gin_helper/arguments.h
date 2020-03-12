// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
#define SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_

#include "gin/arguments.h"
#include "shell/common/gin_helper/v8_type_traits.h"

namespace gin_helper {

// Provides additional APIs to the gin::Arguments class.
class Arguments : public gin::Arguments {
 public:
  // Get the next argument, if conversion to T fails then state is unchanged.
  //
  // This is difference from gin::Arguments::GetNext which always advances the
  // |next_| counter no matter whether the conversion succeeds.
  template <typename T>
  bool GetNext(T* out) {
    if (is_for_property_ || next_ >= info_for_function_->Length()) {
      insufficient_arguments_ = true;
      return false;
    }
    v8::Local<v8::Value> val = (*info_for_function_)[next_];
    if (!gin::ConvertFromV8(isolate(), val, out))
      return false;
    ++next_;
    return true;
  }

  // Gin always returns true when converting V8 value to boolean, we do not want
  // this behavior when parsing parameters.
  bool GetNext(bool* out) {
    if (is_for_property_ || next_ >= info_for_function_->Length()) {
      insufficient_arguments_ = true;
      return false;
    }
    v8::Local<v8::Value> val = (*info_for_function_)[next_];
    if (val.IsEmpty() || !val->IsBoolean())
      return false;
    *out = val->BooleanValue(isolate());
    ++next_;
    return true;
  }

  // Throw arguments error.
  void ThrowError() const;

  // Throw arguments error with expected type information.
  template <typename T>
  void ThrowErrorWithExpectedType() const {
    if (is_for_property_ || insufficient_arguments_ ||
        !V8TypeTraits<T>::has_type_name) {
      ThrowError();
      return;
    }
    ThrowErrorWithExpectedTypeName(V8TypeTraits<T>::type_name);
  }

  // Throw error with custom error message.
  void ThrowError(base::StringPiece message) const;

 private:
  void ThrowErrorWithExpectedTypeName(const char* type_name) const;

  // MUST NOT ADD ANY DATA MEMBER.
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
