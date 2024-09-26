// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/wrappable.h"

#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8-function.h"

namespace gin_helper {

WrappableBase::DisposeObserver::DisposeObserver(
    gin::PerIsolateData* per_isolate_data,
    WrappableBase* wrappable)
    : per_isolate_data_(*per_isolate_data), wrappable_(*wrappable) {
  per_isolate_data_->AddDisposeObserver(this);
}

WrappableBase::DisposeObserver::~DisposeObserver() {
  per_isolate_data_->RemoveDisposeObserver(this);
}

void WrappableBase::DisposeObserver::OnBeforeDispose(v8::Isolate* isolate) {
  if (!wrappable_->wrapper_.IsEmpty()) {
    v8::HandleScope scope(isolate);
    wrappable_->GetWrapper()->SetAlignedPointerInInternalField(0, nullptr);
    wrappable_->wrapper_.ClearWeak();
    wrappable_->wrapper_.Reset();
  }
}

void WrappableBase::DisposeObserver::OnDisposed() {
  // The holder contains the observer, so the observer is destroyed here also.
  delete &wrappable_.get();
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
    return v8::Local<v8::Object>();
}

v8::MaybeLocal<v8::Object> WrappableBase::GetWrapper(
    v8::Isolate* isolate) const {
  if (!wrapper_.IsEmpty())
    return v8::MaybeLocal<v8::Object>(
        v8::Local<v8::Object>::New(isolate, wrapper_));
  else
    return v8::MaybeLocal<v8::Object>();
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
  dispose_observer_ = std::make_unique<DisposeObserver>(
      gin::PerIsolateData::From(isolate), this);
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
  auto* wrappable = static_cast<WrappableBase*>(data.GetInternalField(0));
  if (wrappable) {
    wrappable->wrapper_.Reset();
    data.SetSecondPassCallback(SecondWeakCallback);
  }
}

// static
void WrappableBase::SecondWeakCallback(
    const v8::WeakCallbackInfo<WrappableBase>& data) {
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
