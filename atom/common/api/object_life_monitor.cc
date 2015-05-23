// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/object_life_monitor.h"

#include "native_mate/compat.h"

namespace atom {

// static
void ObjectLifeMonitor::BindTo(v8::Isolate* isolate,
                               v8::Local<v8::Object> target,
                               v8::Local<v8::Value> destructor) {
  target->SetHiddenValue(MATE_STRING_NEW(isolate, "destructor"), destructor);

  ObjectLifeMonitor* olm = new ObjectLifeMonitor();
  olm->handle_.reset(isolate, target);
  olm->handle_.SetWeak(olm, WeakCallback);
}

ObjectLifeMonitor::ObjectLifeMonitor() {
}

// static
void ObjectLifeMonitor::WeakCallback(
    const v8::WeakCallbackData<v8::Object, ObjectLifeMonitor>& data) {
  // destructor.call(object, object);
  v8::Local<v8::Object> obj = data.GetValue();
  v8::Local<v8::Function>::Cast(obj->GetHiddenValue(
      MATE_STRING_NEW(data.GetIsolate(), "destructor")))->Call(obj, 0, NULL);
  delete data.GetParameter();
}

}  // namespace atom
