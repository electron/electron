// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/wrappable.h"

#include "base/logging.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

namespace mate {

Wrappable::Wrappable() {
}

Wrappable::~Wrappable() {
  MATE_PERSISTENT_RESET(wrapper_);
}

void Wrappable::Wrap(v8::Isolate* isolate, v8::Handle<v8::Object> wrapper) {
  if (!wrapper_.IsEmpty())
    return;

  MATE_SET_INTERNAL_FIELD_POINTER(wrapper, 0, this);
  MATE_PERSISTENT_ASSIGN(v8::Object, isolate, wrapper_, wrapper);
  MATE_PERSISTENT_SET_WEAK(wrapper_, this, WeakCallback);

  // Call object._init if we have one.
  v8::Handle<v8::Function> init;
  if (Dictionary(isolate, wrapper).Get("_init", &init))
    init->Call(wrapper, 0, NULL);
}

// static
void Wrappable::BuildPrototype(v8::Isolate* isolate,
                               v8::Handle<v8::ObjectTemplate> prototype) {
}

ObjectTemplateBuilder Wrappable::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return ObjectTemplateBuilder(isolate);
}

// static
MATE_WEAK_CALLBACK(Wrappable::WeakCallback, v8::Object, Wrappable) {
  MATE_WEAK_CALLBACK_INIT(Wrappable);
  MATE_PERSISTENT_RESET(self->wrapper_);
  delete self;
}

v8::Handle<v8::Object> Wrappable::GetWrapper(v8::Isolate* isolate) {
  if (!wrapper_.IsEmpty())
    return MATE_PERSISTENT_TO_LOCAL(v8::Object, isolate, wrapper_);

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
