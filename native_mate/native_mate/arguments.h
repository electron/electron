// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_ARGUMENTS_H_
#define NATIVE_MATE_ARGUMENTS_H_

#include "base/macros.h"
#include "native_mate/converter.h"

namespace mate {

// Arguments is a wrapper around v8::FunctionCallbackInfo that integrates
// with Converter to make it easier to marshall arguments and return values
// between V8 and C++.
class Arguments {
 public:
  Arguments();
  explicit Arguments(const v8::FunctionCallbackInfo<v8::Value>& info);
  ~Arguments();

  v8::Local<v8::Object> GetHolder() const {
    return info_->Holder();
  }

  template<typename T>
  bool GetHolder(T* out) {
    return ConvertFromV8(isolate_, info_->Holder(), out);
  }

  template<typename T>
  bool GetData(T* out) {
    return ConvertFromV8(isolate_, info_->Data(), out);
  }

  template<typename T>
  bool GetNext(T* out) {
    if (next_ >= info_->Length()) {
      insufficient_arguments_ = true;
      return false;
    }
    v8::Local<v8::Value> val = (*info_)[next_];
    bool success = ConvertFromV8(isolate_, val, out);
    if (success)
      next_++;
    return success;
  }

  template<typename T>
  bool GetRemaining(std::vector<T>* out) {
    if (next_ >= info_->Length()) {
      insufficient_arguments_ = true;
      return false;
    }
    int remaining = info_->Length() - next_;
    out->resize(remaining);
    for (int i = 0; i < remaining; ++i) {
      v8::Local<v8::Value> val = (*info_)[next_++];
      if (!ConvertFromV8(isolate_, val, &out->at(i)))
        return false;
    }
    return true;
  }

  v8::Local<v8::Object> GetThis() {
    return info_->This();
  }

  bool IsConstructCall() const {
    return info_->IsConstructCall();
  }

  int Length() const {
    return info_->Length();
  }

  template<typename T>
  void Return(T val) {
    info_->GetReturnValue().Set(ConvertToV8(isolate_, val));
  }

  v8::Local<v8::Value> PeekNext() const;

  v8::Local<v8::Value> ThrowError() const;
  v8::Local<v8::Value> ThrowError(const std::string& message) const;
  v8::Local<v8::Value> ThrowTypeError(const std::string& message) const;

  v8::Isolate* isolate() const { return isolate_; }

 private:
  v8::Isolate* isolate_;
  const v8::FunctionCallbackInfo<v8::Value>* info_;
  int next_;
  bool insufficient_arguments_;
};

}  // namespace mate

#endif  // NATIVE_MATE_ARGUMENTS_H_
