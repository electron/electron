// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_

#include "gin/arguments.h"

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
    v8::Local<v8::Value> val = PeekNext();
    if (val.IsEmpty())
      return false;
    if (!gin::ConvertFromV8(isolate(), val, out))
      return false;
    Skip();
    return true;
  }

  // Gin always returns true when converting V8 value to boolean, we do not want
  // this behavior when parsing parameters.
  bool GetNext(bool* out) {
    v8::Local<v8::Value> val = PeekNext();
    if (val.IsEmpty() || !val->IsBoolean())
      return false;
    *out = val->BooleanValue(isolate());
    Skip();
    return true;
  }

  // Throw error with custom error message.
  void ThrowError() const;
  void ThrowError(base::StringPiece message) const;

 private:
  // MUST NOT ADD ANY DATA MEMBER.
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
