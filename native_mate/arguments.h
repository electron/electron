// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_ARGUMENTS_H_
#define NATIVE_MATE_ARGUMENTS_H_

#include "base/basictypes.h"
#include "native_mate/compat.h"
#include "native_mate/converter.h"

namespace mate {

// Arguments is a wrapper around v8::FunctionCallbackInfo that integrates
// with Converter to make it easier to marshall arguments and return values
// between V8 and C++.
class Arguments {
 public:
  Arguments();
  explicit Arguments(const MATE_METHOD_ARGS_TYPE& info);
  ~Arguments();

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
    v8::Handle<v8::Value> val = (*info_)[next_++];
    return ConvertFromV8(isolate_, val, out);
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
      v8::Handle<v8::Value> val = (*info_)[next_++];
      if (!ConvertFromV8(isolate_, val, &out->at(i)))
        return false;
    }
    return true;
  }

  v8::Handle<v8::Object> GetThis() {
    return info_->This();
  }

  int Length() const {
    return info_->Length();
  }

#if NODE_VERSION_AT_LEAST(0, 11, 0)
  template<typename T>
  void Return(T val) {
    info_->GetReturnValue().Set(ConvertToV8(isolate_, val));
  }
#endif

  v8::Handle<v8::Value> PeekNext() const;

  void ThrowError() const;
  void ThrowTypeError(const std::string& message) const;

  v8::Isolate* isolate() const { return isolate_; }

 private:
  v8::Isolate* isolate_;
  const MATE_METHOD_ARGS_TYPE* info_;
  int next_;
  bool insufficient_arguments_;
};

}  // namespace mate

#endif  // NATIVE_MATE_ARGUMENTS_H_
