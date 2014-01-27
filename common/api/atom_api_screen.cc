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

namespace {

v8::Handle<v8::Object> DisplayToV8Value(const gfx::Display& display) {
  v8::Handle<v8::Object> obj = v8::Object::New();
  obj->Set(ToV8Value("bounds"), ToV8Value(display.bounds()));
  obj->Set(ToV8Value("workArea"), ToV8Value(display.work_area()));
  obj->Set(ToV8Value("size"), ToV8Value(display.size()));
  obj->Set(ToV8Value("workAreaSize"), ToV8Value(display.work_area_size()));
  obj->Set(ToV8Value("scaleFactor"), ToV8Value(display.device_scale_factor()));
  return obj;
}

}  // namespace

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
void Screen::GetPrimaryDisplay(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  UNWRAP_SCREEN_AND_CHECK;
  gfx::Display display = self->screen_->GetPrimaryDisplay();
  args.GetReturnValue().Set(DisplayToV8Value(display));
}

// static
void Screen::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("Screen"));

  NODE_SET_PROTOTYPE_METHOD(t, "getCursorScreenPoint", GetCursorScreenPoint);
  NODE_SET_PROTOTYPE_METHOD(t, "getPrimaryDisplay", GetPrimaryDisplay);

  target->Set(v8::String::NewSymbol("Screen"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_screen, atom::api::Screen::Initialize)
