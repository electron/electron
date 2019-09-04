// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/function_template.h"

namespace gin_helper {

CallbackHolderBase::CallbackHolderBase(v8::Isolate* isolate)
    : v8_ref_(isolate, v8::External::New(isolate, this)) {
  v8_ref_.SetWeak(this, &CallbackHolderBase::FirstWeakCallback,
                  v8::WeakCallbackType::kParameter);
}

CallbackHolderBase::~CallbackHolderBase() {
  DCHECK(v8_ref_.IsEmpty());
}

v8::Local<v8::External> CallbackHolderBase::GetHandle(v8::Isolate* isolate) {
  return v8::Local<v8::External>::New(isolate, v8_ref_);
}

// static
void CallbackHolderBase::FirstWeakCallback(
    const v8::WeakCallbackInfo<CallbackHolderBase>& data) {
  data.GetParameter()->v8_ref_.Reset();
  data.SetSecondPassCallback(SecondWeakCallback);
}

// static
void CallbackHolderBase::SecondWeakCallback(
    const v8::WeakCallbackInfo<CallbackHolderBase>& data) {
  delete data.GetParameter();
}

}  // namespace gin_helper
