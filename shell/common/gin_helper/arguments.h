// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
#define SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_

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

  // Throw error with custom error message.
  void ThrowError(base::StringPiece message) const {
    isolate()->ThrowException(
        v8::Exception::Error(gin::StringToV8(isolate(), message)));
  }

  void ThrowError() const { gin::Arguments::ThrowError(); }

 private:
  // MUST NOT ADD ANY DATA MEMBER.
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_ARGUMENTS_H_
