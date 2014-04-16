// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/wrappable.h"

#include "base/logging.h"
#include "native_mate/object_template_builder.h"

namespace mate {

WrappableBase::WrappableBase() {
}

WrappableBase::~WrappableBase() {
  MATE_PERSISTENT_RESET(wrapper_);
}

void WrappableBase::Wrap(v8::Isolate* isolate, v8::Handle<v8::Object> wrapper) {
  MATE_SET_INTERNAL_FIELD_POINTER(wrapper, 0, this);
  MATE_PERSISTENT_ASSIGN(v8::Object, isolate, wrapper_, wrapper);
  MATE_PERSISTENT_SET_WEAK(wrapper_, this, WeakCallback);
}

ObjectTemplateBuilder WrappableBase::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return ObjectTemplateBuilder(isolate);
}

// static
MATE_WEAK_CALLBACK(WrappableBase::WeakCallback, v8::Object, WrappableBase) {
  MATE_WEAK_CALLBACK_INIT(WrappableBase);
  MATE_PERSISTENT_RESET(self->wrapper_);
  delete self;
}

v8::Handle<v8::Object> WrappableBase::GetWrapperImpl(v8::Isolate* isolate) {
  if (!wrapper_.IsEmpty()) {
    return MATE_PERSISTENT_TO_LOCAL(v8::Object, isolate, wrapper_);
  }

  v8::Local<v8::ObjectTemplate> templ =
      GetObjectTemplateBuilder(isolate).Build();
  CHECK(!templ.IsEmpty());
  CHECK_EQ(1, templ->InternalFieldCount());
  v8::Handle<v8::Object> wrapper = templ->NewInstance();
  Wrap(isolate, wrapper);
  return wrapper;
}

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Handle<v8::Value> val) {
  if (!val->IsObject())
    return NULL;
  v8::Handle<v8::Object> obj = v8::Handle<v8::Object>::Cast(val);
  return MATE_GET_INTERNAL_FIELD_POINTER(obj, 0);
}

}  // namespace internal

}  // namespace mate
