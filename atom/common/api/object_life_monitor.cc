// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/object_life_monitor.h"

namespace atom {

// static
void ObjectLifeMonitor::BindTo(v8::Handle<v8::Object> target,
                               v8::Handle<v8::Value> destructor) {
  target->SetHiddenValue(v8::String::New("destructor"), destructor);

  ObjectLifeMonitor* olm = new ObjectLifeMonitor();
  olm->handle_.reset(target);
  olm->handle_.MakeWeak(olm, WeakCallback);
}

ObjectLifeMonitor::ObjectLifeMonitor() {
}

// static
void ObjectLifeMonitor::WeakCallback(v8::Isolate* isolate,
                                     v8::Persistent<v8::Object>* value,
                                     ObjectLifeMonitor* self) {
  // destructor.call(object, object);
  {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> obj = self->handle_.NewHandle();
    v8::Local<v8::Function>::Cast(obj->GetHiddenValue(
        v8::String::New("destructor")))->Call(obj, 0, NULL);
  }

  delete self;
}

}  // namespace atom
