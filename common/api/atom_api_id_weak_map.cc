// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_id_weak_map.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap()
    : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();

  auto copied_map = map_;
  for (auto el = copied_map.begin(); el != copied_map.end(); ++el)
    Erase(isolate, el->first);
}

bool IDWeakMap::Has(int key) const {
  return map_.find(key) != map_.end();
}

void IDWeakMap::Erase(v8::Isolate* isolate, int key) {
  v8::Persistent<v8::Value> value = map_[key];
  value.ClearWeak(isolate);
  value.Dispose(isolate);
  value.Clear();

  map_.erase(key);
}

int IDWeakMap::GetNextID() {
  return ++next_id_;
}

// static
void IDWeakMap::WeakCallback(v8::Isolate* isolate,
                             v8::Persistent<v8::Value> value,
                             void *data) {
  v8::HandleScope scope;

  IDWeakMap* obj = static_cast<IDWeakMap*>(data);
  int key = value->ToObject()->GetHiddenValue(
      v8::String::New("IDWeakMapKey"))->IntegerValue();
  obj->Erase(isolate, key);
}

// static
v8::Handle<v8::Value> IDWeakMap::New(const v8::Arguments& args) {
  IDWeakMap* obj = new IDWeakMap();
  obj->Wrap(args.This());
  return args.This();
}

// static
v8::Handle<v8::Value> IDWeakMap::Add(const v8::Arguments& args) {
  if (!args[0]->IsObject())
    return node::ThrowTypeError("Bad argument");

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  IDWeakMap* obj = ObjectWrap::Unwrap<IDWeakMap>(args.This());
  int key = obj->GetNextID();

  v8::Handle<v8::Value> v8_key = v8::Integer::New(key);
  v8::Persistent<v8::Value> value =
      v8::Persistent<v8::Value>::New(isolate, args[0]);

  value->ToObject()->SetHiddenValue(v8::String::New("IDWeakMapKey"), v8_key);
  value.MakeWeak(isolate, obj, WeakCallback);
  obj->map_[key] = value;

  return v8_key;
}

// static
v8::Handle<v8::Value> IDWeakMap::Get(const v8::Arguments& args) {
  if (!args[0]->IsNumber())
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* obj = ObjectWrap::Unwrap<IDWeakMap>(args.This());

  int key = args[0]->IntegerValue();
  if (!obj->Has(key))
    return node::ThrowError("Invalid key");

  return obj->map_[key];
}

// static
v8::Handle<v8::Value> IDWeakMap::Has(const v8::Arguments& args) {
  if (!args[0]->IsNumber())
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* obj = ObjectWrap::Unwrap<IDWeakMap>(args.This());

  int key = args[0]->IntegerValue();
  return v8::Boolean::New(obj->Has(key));
}

// static
v8::Handle<v8::Value> IDWeakMap::Keys(const v8::Arguments& args) {
  IDWeakMap* obj = ObjectWrap::Unwrap<IDWeakMap>(args.This());

  v8::Handle<v8::Array> keys = v8::Array::New(obj->map_.size());

  int i = 0;
  for (auto el = obj->map_.begin(); el != obj->map_.end(); ++ el) {
    keys->Set(i, v8::Integer::New(el->first));
    ++i;
  }

  return keys;
}

// static
v8::Handle<v8::Value> IDWeakMap::Remove(const v8::Arguments& args) {
  if (!args[0]->IsNumber())
    return node::ThrowTypeError("Bad argument");

  IDWeakMap* obj = ObjectWrap::Unwrap<IDWeakMap>(args.This());

  int key = args[0]->IntegerValue();
  if (!obj->Has(key))
    return node::ThrowError("Invalid key");

  obj->Erase(v8::Isolate::GetCurrent(), key);
  return v8::Undefined();
}

// static
void IDWeakMap::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(IDWeakMap::New);
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
