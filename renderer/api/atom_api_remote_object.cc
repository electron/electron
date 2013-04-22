// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "renderer/api/atom_api_remote_object.h"

#include "base/logging.h"

namespace atom {

namespace api {

RemoteObject::RemoteObject(v8::Handle<v8::Object> wrapper,
                           const std::string& type)
    : id_(-1) {
  // TODO apply for new object in browser.
  Wrap(wrapper);
}

RemoteObject::RemoteObject(v8::Handle<v8::Object> wrapper, int id)
    : id_(id) {
  Wrap(wrapper);
}

RemoteObject::~RemoteObject() {
  Destroy();
}

void RemoteObject::Destroy() {
  DCHECK(id_ > 0);

  // TODO release me in browser.
}

// static
v8::Handle<v8::Value> RemoteObject::New(const v8::Arguments &args) {
  v8::HandleScope scope;

  if (args[0]->IsString())
    new RemoteObject(args.This(), *v8::String::Utf8Value(args[0]));
  else if (args[0]->IsNumber())
    new RemoteObject(args.This(), args[0]->IntegerValue());
  else
    return node::ThrowTypeError("Bad argument");

  return args.This();
}

// static
v8::Handle<v8::Value> RemoteObject::Destroy(const v8::Arguments &args) {
  RemoteObject* self = ObjectWrap::Unwrap<RemoteObject>(args.This());

  // TODO try to call remote object's destroy method first.

  delete self;

  return v8::Undefined();
}

// static
void RemoteObject::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t =
      v8::FunctionTemplate::New(RemoteObject::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("RemoteObject"));

  NODE_SET_PROTOTYPE_METHOD(t, "destroy", Destroy);

  target->Set(v8::String::NewSymbol("RemoteObject"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_renderer_remote_object, atom::api::RemoteObject::Initialize)
