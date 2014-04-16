// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/object_life_monitor.h"
#include "native_mate/dictionary.h"
#include "v8/include/v8-profiler.h"

#include "atom/common/v8/node_common.h"

namespace {

v8::Handle<v8::Object> CreateObjectWithName(v8::Handle<v8::String> name) {
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
  t->SetClassName(name);
  return t->GetFunction()->NewInstance();
}

v8::Handle<v8::Value> GetHiddenValue(v8::Handle<v8::Object> object,
                                     v8::Handle<v8::String> key) {
  return object->GetHiddenValue(key);
}

void SetHiddenValue(v8::Handle<v8::Object> object,
                    v8::Handle<v8::String> key,
                    v8::Handle<v8::Value> value) {
  object->SetHiddenValue(key, value);
}

int32_t GetObjectHash(v8::Handle<v8::Object> object) {
  return object->GetIdentityHash();
}

void SetDestructor(v8::Handle<v8::Object> object,
                   v8::Handle<v8::Function> callback) {
  atom::ObjectLifeMonitor::BindTo(object, callback);
}

void TakeHeapSnapshot() {
  node::node_isolate->GetHeapProfiler()->TakeHeapSnapshot(
      v8::String::New("test"));
}

void Initialize(v8::Handle<v8::Object> exports) {
  mate::Dictionary dict(v8::Isolate::GetCurrent(), exports);
  dict.SetMethod("createObjectWithName", &CreateObjectWithName);
  dict.SetMethod("getHiddenValue", &GetHiddenValue);
  dict.SetMethod("setHiddenValue", &SetHiddenValue);
  dict.SetMethod("getObjectHash", &GetObjectHash);
  dict.SetMethod("setDestructor", &SetDestructor);
  dict.SetMethod("takeHeapSnapshot", &TakeHeapSnapshot);
}

}  // namespace

NODE_MODULE(atom_common_v8_util, Initialize)
