// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/wrappable.h"

#include "base/logging.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

namespace mate {

Wrappable::Wrappable() : isolate_(NULL) {
}

Wrappable::~Wrappable() {
  if (!wrapper_.IsEmpty())
    GetWrapper(isolate())->SetAlignedPointerInInternalField(0, nullptr);
  wrapper_.Reset();
}

void Wrappable::Wrap(v8::Isolate* isolate, v8::Local<v8::Object> wrapper) {
  if (!wrapper_.IsEmpty())
    return;

  isolate_ = isolate;

  wrapper->SetAlignedPointerInInternalField(0, this);
  wrapper_.Reset(isolate, wrapper);
  wrapper_.SetWeak(this, FirstWeakCallback, v8::WeakCallbackType::kParameter);

  // Call object._init if we have one.
  v8::Local<v8::Function> init;
  if (Dictionary(isolate, wrapper).Get("_init", &init))
    init->Call(wrapper, 0, nullptr);

  AfterInit(isolate);
}

// static
void Wrappable::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::ObjectTemplate> prototype) {
}

ObjectTemplateBuilder Wrappable::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return ObjectTemplateBuilder(isolate);
}

void Wrappable::FirstWeakCallback(const v8::WeakCallbackInfo<Wrappable>& data) {
  Wrappable* wrappable = data.GetParameter();
  wrappable->wrapper_.Reset();
  data.SetSecondPassCallback(SecondWeakCallback);
}

void Wrappable::SecondWeakCallback(
    const v8::WeakCallbackInfo<Wrappable>& data) {
  Wrappable* wrappable = data.GetParameter();
  delete wrappable;
}

v8::Local<v8::Object> Wrappable::GetWrapper(v8::Isolate* isolate) {
  if (!wrapper_.IsEmpty())
    return MATE_PERSISTENT_TO_LOCAL(v8::Object, isolate, wrapper_);

  v8::Local<v8::ObjectTemplate> templ =
      GetObjectTemplateBuilder(isolate).Build();
  CHECK(!templ.IsEmpty());
  CHECK_EQ(1, templ->InternalFieldCount());
  v8::Local<v8::Object> wrapper;
  // |wrapper| may be empty in some extreme cases, e.g., when
  // Object.prototype.constructor is overwritten.
  if (!templ->NewInstance(isolate->GetCurrentContext()).ToLocal(&wrapper)) {
    // The current wrappable object will be no longer managed by V8. Delete this
    // now.
    delete this;
    return wrapper;
  }
  Wrap(isolate, wrapper);
  return wrapper;
}

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  if (!val->IsObject())
    return NULL;
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(val);
  if (obj->InternalFieldCount() != 1)
    return NULL;
  return MATE_GET_INTERNAL_FIELD_POINTER(obj, 0);
}

}  // namespace internal

}  // namespace mate
