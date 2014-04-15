// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/constructor.h"

#include "native_mate/function_template.h"
#include "native_mate/object_template_builder.h"

namespace mate {

Constructor::Constructor(const base::StringPiece& name) : name_(name) {
}

Constructor::~Constructor() {
  constructor_.Reset();
}

v8::Handle<v8::Function> Constructor::GetFunction(v8::Isolate* isolate) {
  if (constructor_.IsEmpty()) {
    v8::Local<v8::FunctionTemplate> constructor = CreateFunctionTemplate(
        isolate,
        base::Bind(&Constructor::New, base::Unretained(this)));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(StringToV8(isolate, name_));
    SetPrototype(isolate, constructor->PrototypeTemplate());

    constructor_.Reset(isolate, constructor);
  }

  return MATE_PERSISTENT_TO_LOCAL(
      v8::FunctionTemplate, isolate, constructor_)->GetFunction();
}

void Constructor::New() {
}

void Constructor::SetPrototype(v8::Isolate* isolate,
                               v8::Handle<v8::ObjectTemplate> prototype) {
}

}  // namespace mate
