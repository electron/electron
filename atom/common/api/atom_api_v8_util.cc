// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/object_life_monitor.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"
#include "v8/include/v8-profiler.h"

namespace {

v8::Local<v8::Object> CreateObjectWithName(v8::Isolate* isolate,
                                            v8::Local<v8::String> name) {
  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(isolate);
  t->SetClassName(name);
  return t->GetFunction()->NewInstance();
}

v8::Local<v8::Value> GetHiddenValue(v8::Local<v8::Object> object,
                                     v8::Local<v8::String> key) {
  return object->GetHiddenValue(key);
}

void SetHiddenValue(v8::Local<v8::Object> object,
                    v8::Local<v8::String> key,
                    v8::Local<v8::Value> value) {
  object->SetHiddenValue(key, value);
}

void DeleteHiddenValue(v8::Local<v8::Object> object,
                       v8::Local<v8::String> key) {
  object->DeleteHiddenValue(key);
}

int32_t GetObjectHash(v8::Local<v8::Object> object) {
  return object->GetIdentityHash();
}

void SetDestructor(v8::Isolate* isolate,
                   v8::Local<v8::Object> object,
                   v8::Local<v8::Function> callback) {
  atom::ObjectLifeMonitor::BindTo(isolate, object, callback);
}

void TakeHeapSnapshot(v8::Isolate* isolate) {
  isolate->GetHeapProfiler()->TakeHeapSnapshot();
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createObjectWithName", &CreateObjectWithName);
  dict.SetMethod("getHiddenValue", &GetHiddenValue);
  dict.SetMethod("setHiddenValue", &SetHiddenValue);
  dict.SetMethod("deleteHiddenValue", &DeleteHiddenValue);
  dict.SetMethod("getObjectHash", &GetObjectHash);
  dict.SetMethod("setDestructor", &SetDestructor);
  dict.SetMethod("takeHeapSnapshot", &TakeHeapSnapshot);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_v8_util, Initialize)
