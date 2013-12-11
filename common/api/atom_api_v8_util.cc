// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/object_life_monitor.h"

#include "common/v8/native_type_conversions.h"
#include "v8/include/v8-profiler.h"

#include "common/v8/node_common.h"

namespace atom {

namespace api {

namespace {

void CreateObjectWithName(const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  t->SetClassName(args[0]->ToString());
  args.GetReturnValue().Set(t->GetFunction()->NewInstance());
}

void GetHiddenValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(
      args[0]->ToObject()->GetHiddenValue(args[1]->ToString()));
}

void SetHiddenValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args[0]->ToObject()->SetHiddenValue(args[1]->ToString(), args[2]);
}

void GetObjectHash(const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(args[0]->ToObject()->GetIdentityHash());
}

void SetDestructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
  ObjectLifeMonitor::BindTo(args[0]->ToObject(), args[1]);
}

void TakeHeapSnapshot(const v8::FunctionCallbackInfo<v8::Value>& args) {
  node::node_isolate->GetHeapProfiler()->TakeHeapSnapshot(
      v8::String::New("test"));
}

}  // namespace

void InitializeV8Util(v8::Handle<v8::Object> target) {
  v8::HandleScope handle_scope(node_isolate);

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
