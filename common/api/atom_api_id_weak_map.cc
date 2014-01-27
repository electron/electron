// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_id_weak_map.h"

#include <algorithm>

#include "base/logging.h"
#include "common/v8/native_type_conversions.h"
#include "common/v8/node_common.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap()
    : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
}

bool IDWeakMap::Has(int key) const {
  return map_.find(key) != map_.end();
}

void IDWeakMap::Erase(int key) {
  if (Has(key))
    map_.erase(key);
  else
    LOG(WARNING) << "Object with key " << key << " is being GCed for twice.";
}

int IDWeakMap::GetNextID() {
  return ++next_id_;
}

// static
void IDWeakMap::WeakCallback(v8::Isolate* isolate,
                             v8::Persistent<v8::Object>* value,
                             IDWeakMap* self) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> local = v8::Local<v8::Object>::New(isolate, *value);
  self->Erase(
      FromV8Value(local->GetHiddenValue(v8::String::New("IDWeakMapKey"))));
}

// static
void IDWeakMap::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  (new IDWeakMap)->Wrap(args.This());
}

// static
void IDWeakMap::Add(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args[0]->IsObject())
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* self = Unwrap<IDWeakMap>(args.This());

  int key = self->GetNextID();
  v8::Local<v8::Object> v8_value = args[0]->ToObject();
  v8_value->SetHiddenValue(v8::String::New("IDWeakMapKey"), ToV8Value(key));

  self->map_[key] = new RefCountedPersistent<v8::Object>(v8_value);
  self->map_[key]->MakeWeak(self, WeakCallback);

  args.GetReturnValue().Set(key);
}

// static
void IDWeakMap::Get(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int key;
  if (!FromV8Arguments(args, &key))
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* self = Unwrap<IDWeakMap>(args.This());
  if (!self->Has(key))
    return node::ThrowError("Invalid key");

  args.GetReturnValue().Set(self->map_[key]->NewHandle());
}

// static
void IDWeakMap::Has(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int key;
  if (!FromV8Arguments(args, &key))
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* self = Unwrap<IDWeakMap>(args.This());
  args.GetReturnValue().Set(self->Has(key));
}

// static
void IDWeakMap::Keys(const v8::FunctionCallbackInfo<v8::Value>& args) {
  IDWeakMap* self = Unwrap<IDWeakMap>(args.This());

  v8::Local<v8::Array> keys = v8::Array::New(self->map_.size());

  int i = 0;
  for (auto el = self->map_.begin(); el != self->map_.end(); ++el) {
    keys->Set(i, ToV8Value(el->first));
    ++i;
  }

  args.GetReturnValue().Set(keys);
}

// static
void IDWeakMap::Remove(const v8::FunctionCallbackInfo<v8::Value>& args) {
  int key;
  if (!FromV8Arguments(args, &key))
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* self = Unwrap<IDWeakMap>(args.This());
  if (!self->Has(key))
    return node::ThrowError("Invalid key");

  self->Erase(key);
}

// static
void IDWeakMap::Initialize(v8::Handle<v8::Object> target) {
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("IDWeakMap"));

  NODE_SET_PROTOTYPE_METHOD(t, "add", Add);
  NODE_SET_PROTOTYPE_METHOD(t, "get", Get);
  NODE_SET_PROTOTYPE_METHOD(t, "has", Has);
  NODE_SET_PROTOTYPE_METHOD(t, "keys", Keys);
  NODE_SET_PROTOTYPE_METHOD(t, "remove", Remove);

  target->Set(v8::String::NewSymbol("IDWeakMap"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_id_weak_map, atom::api::IDWeakMap::Initialize)
