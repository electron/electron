// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/object_life_monitor.h"
#include "v8/include/v8-profiler.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

namespace {

v8::Handle<v8::Value> DummyNew(const v8::Arguments& args) {
  return args.This();
}

v8::Handle<v8::Value> CreateObjectWithName(const v8::Arguments& args) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(DummyNew);
  t->SetClassName(args[0]->ToString());

  return scope.Close(t->GetFunction()->NewInstance());
}

v8::Handle<v8::Value> GetHiddenValue(const v8::Arguments& args) {
  return args[0]->ToObject()->GetHiddenValue(args[1]->ToString());
}

v8::Handle<v8::Value> SetHiddenValue(const v8::Arguments& args) {
  args[0]->ToObject()->SetHiddenValue(args[1]->ToString(), args[2]);
  return v8::Undefined();
}

v8::Handle<v8::Value> GetObjectHash(const v8::Arguments& args) {
  v8::HandleScope handle_scope;
  return handle_scope.Close(v8::Integer::New(
      args[0]->ToObject()->GetIdentityHash()));
}

v8::Handle<v8::Value> SetDestructor(const v8::Arguments& args) {
  ObjectLifeMonitor::BindTo(args[0]->ToObject(), args[1]);
  return v8::Undefined();
}

v8::Handle<v8::Value> TakeHeapSnapshot(const v8::Arguments& args) {
  node::node_isolate->GetHeapProfiler()->TakeHeapSnapshot(
      v8::String::New("test"));
  return v8::Undefined();
}

}  // namespace

void InitializeV8Util(v8::Handle<v8::Object> target) {
  NODE_SET_METHOD(target, "createObjectWithName", CreateObjectWithName);
  NODE_SET_METHOD(target, "getHiddenValue", GetHiddenValue);
  NODE_SET_METHOD(target, "setHiddenValue", SetHiddenValue);
  NODE_SET_METHOD(target, "getObjectHash", GetObjectHash);
  NODE_SET_METHOD(target, "setDestructor", SetDestructor);
  NODE_SET_METHOD(target, "takeHeapSnapshot", TakeHeapSnapshot);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_v8_util, atom::api::InitializeV8Util)
