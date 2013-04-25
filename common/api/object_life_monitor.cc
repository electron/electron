// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/object_life_monitor.h"

namespace atom {

// static
void ObjectLifeMonitor::BindTo(v8::Handle<v8::Object> target,
                               v8::Handle<v8::Value> destructor) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  target->SetHiddenValue(v8::String::New("destructor"), destructor);

  ObjectLifeMonitor* olm = new ObjectLifeMonitor();
  olm->handle_ = v8::Persistent<v8::Object>::New(isolate, target);
  olm->handle_.MakeWeak(isolate, olm, WeakCallback);
}

ObjectLifeMonitor::ObjectLifeMonitor() {
}

ObjectLifeMonitor::~ObjectLifeMonitor() {
  if (!handle_.IsEmpty()) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    handle_.ClearWeak(isolate);
    handle_.Dispose(isolate);
    handle_.Clear();
  }
}

// static
void ObjectLifeMonitor::WeakCallback(v8::Isolate* isolate,
                                     v8::Persistent<v8::Value> value,
                                     void *data) {
  // destructor.call(object, object);
  {
    v8::HandleScope scope;

    v8::Local<v8::Object> obj = value->ToObject();
    v8::Local<v8::Value> args[] = { obj };
    v8::Local<v8::Function>::Cast(obj->GetHiddenValue(
        v8::String::New("destructor")))->Call(obj, 1, args);
  }

  ObjectLifeMonitor* obj = static_cast<ObjectLifeMonitor*>(data);
  delete obj;
}

}  // namespace atom
