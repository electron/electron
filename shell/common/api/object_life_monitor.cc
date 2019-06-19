// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/object_life_monitor.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"

namespace electron {

ObjectLifeMonitor::ObjectLifeMonitor(v8::Isolate* isolate,
                                     v8::Local<v8::Object> target)
    : target_(isolate, target), weak_ptr_factory_(this) {
  target_.SetWeak(this, OnObjectGC, v8::WeakCallbackType::kParameter);
}

ObjectLifeMonitor::~ObjectLifeMonitor() {
  if (target_.IsEmpty())
    return;
  target_.ClearWeak();
  target_.Reset();
}

// static
void ObjectLifeMonitor::OnObjectGC(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  ObjectLifeMonitor* self = data.GetParameter();
  self->target_.Reset();
  self->RunDestructor();
  data.SetSecondPassCallback(Free);
}

// static
void ObjectLifeMonitor::Free(
    const v8::WeakCallbackInfo<ObjectLifeMonitor>& data) {
  delete data.GetParameter();
}

}  // namespace electron
