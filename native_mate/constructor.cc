// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/constructor.h"

#include "base/bind.h"
#include "base/strings/string_piece.h"
#include "native_mate/arguments.h"
#include "native_mate/function_template.h"

namespace mate {

Constructor::Constructor(const base::StringPiece& name,
                         const WrappableFactoryFunction& factory)
    : name_(name), factory_(factory) {
}

virtual Constructor::~Constructor() {
  constructor_.Reset();
}

v8::Handle<v8::FunctionTemplate> Constructor::GetFunctionTemplate(
    v8::Isolate* isolate) {
  if (constructor_.IsEmpty()) {
    v8::Local<v8::FunctionTemplate> constructor = CreateFunctionTemplate(
        isolate, base::Bind(&Constructor::New, base::Unretained(this)));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(StringToV8(isolate, name_));
    constructor_.Reset(isolate, constructor);
  }

  return MATE_PERSISTENT_TO_LOCAL(v8::FunctionTemplate, isolate, constructor_);
}

void Constructor::New(mate::Arguments* args) {
  MATE_SET_INTERNAL_FIELD_POINTER(args->GetThis(), 0, factory_.Run());
}

}  // namespace mate
