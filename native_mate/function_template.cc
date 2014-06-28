// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/function_template.h"

namespace mate {

namespace internal {

CallbackHolderBase::CallbackHolderBase(v8::Isolate* isolate)
    : MATE_PERSISTENT_INIT(isolate, v8_ref_, MATE_EXTERNAL_NEW(isolate, this)) {
  MATE_PERSISTENT_SET_WEAK(v8_ref_, this, &CallbackHolderBase::WeakCallback);
}

CallbackHolderBase::~CallbackHolderBase() {
  DCHECK(v8_ref_.IsEmpty());
}

v8::Handle<v8::External> CallbackHolderBase::GetHandle(v8::Isolate* isolate) {
  return MATE_PERSISTENT_TO_LOCAL(v8::External, isolate, v8_ref_);
}

// static
MATE_WEAK_CALLBACK(CallbackHolderBase::WeakCallback,
                   v8::External,
                   CallbackHolderBase) {
  MATE_WEAK_CALLBACK_INIT(CallbackHolderBase);
  MATE_PERSISTENT_RESET(self->v8_ref_);
  delete self;
}

}  // namespace internal

}  // namespace mate
