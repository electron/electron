// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_
#define ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_

#include "browser/api/atom_api_recorded_object.h"

#include "base/compiler_specific.h"
#include "browser/api/atom_api_objects_registry.h"
#include "browser/atom_browser_context.h"

namespace atom {

namespace api {

RecordedObject::RecordedObject(v8::Handle<v8::Object> wrapper)
    : ALLOW_THIS_IN_INITIALIZER_LIST(id_(
          AtomBrowserContext::Get()->objects_registry()->Add(this))) {
  Wrap(wrapper);

  wrapper->SetAccessor(v8::String::New("id"), IDGetter);
}

RecordedObject::~RecordedObject() {
  AtomBrowserContext::Get()->objects_registry()->Remove(id());
}

// static
v8::Handle<v8::Value> RecordedObject::IDGetter(v8::Local<v8::String> property,
                                               const v8::AccessorInfo& info) {
  RecordedObject* self = RecordedObject::Unwrap<RecordedObject>(info.This());
  return v8::Integer::New(self->id_);
}

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_
