// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_event.h"

using node::node_isolate;

namespace atom {

namespace api {

v8::Persistent<v8::FunctionTemplate> Event::constructor_template_;

Event::Event()
    : prevent_default_(false) {
}

Event::~Event() {
}

// static
v8::Handle<v8::Object> Event::CreateV8Object() {
  v8::HandleScope scope;

  if (constructor_template_.IsEmpty()) {
    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
    constructor_template_ = v8::Persistent<v8::FunctionTemplate>::New(
        node_isolate, t);
    constructor_template_->InstanceTemplate()->SetInternalFieldCount(1);
    constructor_template_->SetClassName(v8::String::NewSymbol("Event"));

    NODE_SET_PROTOTYPE_METHOD(t, "preventDefault", PreventDefault);
  }

  v8::Handle<v8::Object> v8_event =
      constructor_template_->GetFunction()->NewInstance(0, NULL);

  return scope.Close(v8_event);
}

v8::Handle<v8::Value> Event::New(const v8::Arguments &args) {
  Event* event = new Event;
  event->Wrap(args.This());

  return args.This();
}

v8::Handle<v8::Value> Event::PreventDefault(const v8::Arguments &args) {
  Event* event = Unwrap<Event>(args.This());
  event->prevent_default_ = true;

  return v8::Undefined();
}

}  // namespace api

}  // namespace atom
