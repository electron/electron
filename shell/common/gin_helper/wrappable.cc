// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/wrappable.h"

#include "gin/public/isolate_holder.h"
#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8-function.h"

namespace gin_helper {

bool IsValidWrappable(const v8::Local<v8::Value>& val,
                      const gin::WrapperInfo* wrapper_info) {
  if (!val->IsObject())
    return false;

  v8::Local<v8::Object> port = val.As<v8::Object>();
  if (port->InternalFieldCount() != gin::kNumberOfInternalFields)
    return false;

  const gin::WrapperInfo* info = static_cast<gin::WrapperInfo*>(
      port->GetAlignedPointerFromInternalField(gin::kWrapperInfoIndex));
  if (info != wrapper_info)
    return false;

  return true;
}

WrappableBase::WrappableBase() = default;

WrappableBase::~WrappableBase() {
  if (wrapper_.IsEmpty())
    return;

  v8::HandleScope scope(isolate());
  GetWrapper()->SetAlignedPointerInInternalField(0, nullptr);
  wrapper_.ClearWeak();
  wrapper_.Reset();
}

v8::Local<v8::Object> WrappableBase::GetWrapper() const {
  if (!wrapper_.IsEmpty())
    return v8::Local<v8::Object>::New(isolate_, wrapper_);
  else
    return {};
}

void WrappableBase::InitWithArgs(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  args->GetHolder(&holder);
  InitWith(args->isolate(), holder);
}

void WrappableBase::InitWith(v8::Isolate* isolate,
                             v8::Local<v8::Object> wrapper) {
  CHECK(wrapper_.IsEmpty());
  isolate_ = isolate;
  wrapper->SetAlignedPointerInInternalField(0, this);
  wrapper_.Reset(isolate, wrapper);
  wrapper_.SetWeak(this, FirstWeakCallback,
                   v8::WeakCallbackType::kInternalFields);

  // Call object._init if we have one.
  v8::Local<v8::Function> init;
  if (Dictionary(isolate, wrapper).Get("_init", &init))
    init->Call(isolate->GetCurrentContext(), wrapper, 0, nullptr).IsEmpty();
}

// static
void WrappableBase::FirstWeakCallback(
    const v8::WeakCallbackInfo<WrappableBase>& data) {
  WrappableBase* wrappable = data.GetParameter();
  auto* wrappable_from_field =
      static_cast<WrappableBase*>(data.GetInternalField(0));
  if (wrappable && wrappable == wrappable_from_field) {
    wrappable->wrapper_.Reset();
    data.SetSecondPassCallback(SecondWeakCallback);
  }
}

// static
void WrappableBase::SecondWeakCallback(
    const v8::WeakCallbackInfo<WrappableBase>& data) {
  if (gin::IsolateHolder::DestroyedMicrotasksRunner()) {
    return;
  }
  delete static_cast<WrappableBase*>(data.GetInternalField(0));
}

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  if (!val->IsObject())
    return nullptr;
  v8::Local<v8::Object> obj = val.As<v8::Object>();
  if (obj->InternalFieldCount() != 1)
    return nullptr;
  return obj->GetAlignedPointerFromInternalField(0);
}

}  // namespace internal

}  // namespace gin_helper
