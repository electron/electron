// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "shell/common/gin_helper/wrappable.h"

#include "base/task/sequenced_task_runner.h"
#include "gin/public/isolate_holder.h"
#include "shell/common/gin_helper/dictionary.h"
#include "v8/include/v8-function.h"

namespace gin_helper {

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
  // Defer destruction to a posted task. V8's second-pass weak callbacks run
  // inside a DisallowJavascriptExecutionScope (they may touch the V8 API but
  // must not invoke JS). Several Electron Wrappables (e.g. WebContents) emit
  // JS events from their destructors, so deleting synchronously here can
  // crash with "Invoke in DisallowJavascriptExecutionScope" — see
  // https://github.com/electron/electron/issues/47420. Posting via the
  // current sequence's task runner ensures the destructor runs once V8 has
  // left the GC scope. If no task runner is available (e.g. early/late in
  // process lifetime), fall back to synchronous deletion.
  auto* wrappable = static_cast<WrappableBase*>(data.GetInternalField(0));
  if (base::SequencedTaskRunner::HasCurrentDefault()) {
    base::SequencedTaskRunner::GetCurrentDefault()->DeleteSoon(FROM_HERE,
                                                               wrappable);
  } else {
    delete wrappable;
  }
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

}  // namespace internal

}  // namespace gin_helper
