// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_screen.h"

#include "common/v8/native_type_conversions.h"
#include "ui/gfx/screen.h"

#include "common/v8/node_common.h"

#define UNWRAP_SCREEN_AND_CHECK \
  Screen* self = ObjectWrap::Unwrap<Screen>(args.This()); \
  if (self == NULL) \
    return node::ThrowError("Screen is already destroyed")

namespace atom {

namespace api {

Screen::Screen(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper),
      screen_(gfx::Screen::GetNativeScreen()) {
}

Screen::~Screen() {
}

// static
void Screen::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::HandleScope scope(args.GetIsolate());

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  new Screen(args.This());
}

// static
void Screen::GetCursorScreenPoint(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_SCREEN_AND_CHECK;
  args.GetReturnValue().Set(ToV8Value(self->screen_->GetCursorScreenPoint()));
}

// static
void Screen::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Screen"));

  NODE_SET_PROTOTYPE_METHOD(t, "getCursorScreenPoint", GetCursorScreenPoint);

  target->Set(v8::String::NewSymbol("Screen"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_screen, atom::api::Screen::Initialize)
