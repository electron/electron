// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/object_life_monitor.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"

namespace atom {

// static
void ObjectLifeMonitor::BindTo(v8::Isolate* isolate,
                               v8::Local<v8::Object> target,
                               v8::Local<v8::Function> destructor) {
  new ObjectLifeMonitor(isolate, target, destructor);
}

ObjectLifeMonitor::ObjectLifeMonitor(v8::Isolate* isolate,
                                     v8::Local<v8::Object> target,
                                     v8::Local<v8::Function> destructor)
    : isolate_(isolate),
      context_(isolate, isolate->GetCurrentContext()),
      target_(isolate, target),
      destructor_(isolate, destructor),
      weak_ptr_factory_(this) {
  target_.SetWeak(this, OnObjectGC, v8::WeakCallbackType::kParameter);
}

// static
void ObjectLifeMonitor::OnObjectGC(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  ObjectLifeMonitor* self = data.GetParameter();
  self->target_.Reset();
  self->RunCallback();
  data.SetSecondPassCallback(Free);
}

// static
void ObjectLifeMonitor::Free(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  delete data.GetParameter();
}

void ObjectLifeMonitor::RunCallback() {
  v8::HandleScope handle_scope(isolate_);
  v8::Local<v8::Context> context = v8::Local<v8::Context>::New(
      isolate_, context_);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::Function>::New(isolate_, destructor_)->Call(
      context->Global(), 0, nullptr);
}

}  // namespace atom
