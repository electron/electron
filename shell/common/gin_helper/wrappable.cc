// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/wrappable.h"

#include "gin/object_template_builder.h"
#include "gin/public/isolate_holder.h"
#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8-function.h"

namespace gin_helper {

bool IsValidWrappable(const v8::Local<v8::Value>& val,
                      const gin::DeprecatedWrapperInfo* wrapper_info) {
  if (!val->IsObject())
    return false;

  v8::Local<v8::Object> port = val.As<v8::Object>();
  if (port->InternalFieldCount() != gin::kNumberOfInternalFields)
    return false;

  const gin::DeprecatedWrapperInfo* info =
      static_cast<gin::DeprecatedWrapperInfo*>(
          port->GetAlignedPointerFromInternalField(
              gin::kWrapperInfoIndex, v8::kEmbedderDataTypeTagDefault));
  if (info != wrapper_info)
    return false;

  return true;
}

WrappableBase::WrappableBase() = default;

WrappableBase::~WrappableBase() {
  if (wrapper_.IsEmpty())
    return;

  v8::HandleScope scope(isolate());
  GetWrapper()->SetAlignedPointerInInternalField(
      0, nullptr, v8::kEmbedderDataTypeTagDefault);
  wrapper_.ClearWeak();
  wrapper_.Reset();
}

v8::Local<v8::Object> WrappableBase::GetWrapper() const {
  if (!wrapper_.IsEmpty())
    return v8::Local<v8::Object>::New(isolate_, wrapper_);
  else
    return {};
}

void WrappableBase::InitWithArgs(const gin::Arguments* const args) {
  v8::Local<v8::Object> holder;
  args->GetHolder(&holder);
  InitWith(args->isolate(), holder);
}

void WrappableBase::InitWith(v8::Isolate* isolate,
                             v8::Local<v8::Object> wrapper) {
  CHECK(wrapper_.IsEmpty());
  isolate_ = isolate;
  wrapper->SetAlignedPointerInInternalField(0, this,
                                            v8::kEmbedderDataTypeTagDefault);
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

DeprecatedWrappableBase::DeprecatedWrappableBase() = default;

DeprecatedWrappableBase::~DeprecatedWrappableBase() {
  if (!wrapper_.IsEmpty())
    wrapper_.ClearWeak();
  wrapper_.Reset();
}

gin::ObjectTemplateBuilder DeprecatedWrappableBase::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::ObjectTemplateBuilder(isolate, GetTypeName());
}

const char* DeprecatedWrappableBase::GetTypeName() {
  return nullptr;
}

void DeprecatedWrappableBase::FirstWeakCallback(
    const v8::WeakCallbackInfo<DeprecatedWrappableBase>& data) {
  DeprecatedWrappableBase* wrappable = data.GetParameter();
  DeprecatedWrappableBase* wrappable_from_field =
      static_cast<DeprecatedWrappableBase*>(data.GetInternalField(1));
  if (wrappable && wrappable == wrappable_from_field) {
    wrappable->dead_ = true;
    wrappable->wrapper_.Reset();
    data.SetSecondPassCallback(SecondWeakCallback);
  }
}

void DeprecatedWrappableBase::SecondWeakCallback(
    const v8::WeakCallbackInfo<DeprecatedWrappableBase>& data) {
  if (gin::IsolateHolder::DestroyedMicrotasksRunner())
    return;
  DeprecatedWrappableBase* wrappable = data.GetParameter();
  if (wrappable)
    delete wrappable;
}

v8::MaybeLocal<v8::Object> DeprecatedWrappableBase::GetWrapperImpl(
    v8::Isolate* isolate,
    gin::DeprecatedWrapperInfo* info) {
  if (!wrapper_.IsEmpty()) {
    return v8::MaybeLocal<v8::Object>(
        v8::Local<v8::Object>::New(isolate, wrapper_));
  }

  if (dead_) {
    return v8::MaybeLocal<v8::Object>();
  }

  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::ObjectTemplate> templ = data->DeprecatedGetObjectTemplate(info);
  if (templ.IsEmpty()) {
    templ = GetObjectTemplateBuilder(isolate).Build();
    CHECK(!templ.IsEmpty());
    data->DeprecatedSetObjectTemplate(info, templ);
  }
  CHECK_EQ(gin::kNumberOfInternalFields, templ->InternalFieldCount());
  v8::Local<v8::Object> wrapper;
  // |wrapper| may be empty in some extreme cases, e.g., when
  // Object.prototype.constructor is overwritten.
  if (!templ->NewInstance(isolate->GetCurrentContext()).ToLocal(&wrapper)) {
    // The current wrappable object will be no longer managed by V8. Delete this
    // now.
    delete this;
    return v8::MaybeLocal<v8::Object>(wrapper);
  }

  wrapper->SetAlignedPointerInInternalField(gin::kWrapperInfoIndex, info,
                                            v8::kEmbedderDataTypeTagDefault);
  wrapper->SetAlignedPointerInInternalField(gin::kEncodedValueIndex, this,
                                            v8::kEmbedderDataTypeTagDefault);
  wrapper_.Reset(isolate, wrapper);
  wrapper_.SetWeak(this, FirstWeakCallback,
                   v8::WeakCallbackType::kInternalFields);
  return v8::MaybeLocal<v8::Object>(wrapper);
}

void DeprecatedWrappableBase::ClearWeak() {
  if (!wrapper_.IsEmpty())
    wrapper_.ClearWeak();
}

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  if (!val->IsObject())
    return nullptr;
  v8::Local<v8::Object> obj = val.As<v8::Object>();
  if (obj->InternalFieldCount() != 1)
    return nullptr;
  return obj->GetAlignedPointerFromInternalField(
      0, v8::kEmbedderDataTypeTagDefault);
}

void* FromV8Impl(v8::Isolate* isolate,
                 v8::Local<v8::Value> val,
                 gin::DeprecatedWrapperInfo* wrapper_info) {
  if (!val->IsObject()) {
    return nullptr;
  }
  v8::Local<v8::Object> obj = v8::Local<v8::Object>::Cast(val);
  gin::DeprecatedWrapperInfo* info = gin::DeprecatedWrapperInfo::From(obj);

  // If this fails, the object is not managed by Gin. It is either a normal JS
  // object that's not wrapping any external C++ object, or it is wrapping some
  // C++ object, but that object isn't managed by Gin (maybe Blink).
  if (!info) {
    return nullptr;
  }

  // If this fails, the object is managed by Gin, but it's not wrapping an
  // instance of the C++ class associated with wrapper_info.
  if (info != wrapper_info) {
    return nullptr;
  }

  return obj->GetAlignedPointerFromInternalField(
      gin::kEncodedValueIndex, v8::kEmbedderDataTypeTagDefault);
}

}  // namespace internal

}  // namespace gin_helper

namespace gin {

DeprecatedWrapperInfo* DeprecatedWrapperInfo::From(
    v8::Local<v8::Object> object) {
  if (object->InternalFieldCount() != kNumberOfInternalFields)
    return NULL;
  DeprecatedWrapperInfo* info = static_cast<DeprecatedWrapperInfo*>(
      object->GetAlignedPointerFromInternalField(
          kWrapperInfoIndex, v8::kEmbedderDataTypeTagDefault));
  return info->embedder == kEmbedderNativeGin ? info : NULL;
}

}  // namespace gin
