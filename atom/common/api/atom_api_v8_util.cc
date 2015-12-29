// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "atom/common/api/object_life_monitor.h"
#include "atom/common/node_includes.h"
#include "base/stl_util.h"
#include "native_mate/dictionary.h"
#include "v8/include/v8-profiler.h"

namespace {

// A Persistent that can be copied and will not free itself.
template<class T>
struct LeakedPersistentTraits {
  typedef v8::Persistent<T, LeakedPersistentTraits<T> > LeakedPersistent;
  static const bool kResetInDestructor = false;
  template<class S, class M>
  static V8_INLINE void Copy(const v8::Persistent<S, M>& source,
                             LeakedPersistent* dest) {
    // do nothing, just allow copy
  }
};

// The handles are leaked on purpose.
using FunctionTemplateHandle =
    LeakedPersistentTraits<v8::FunctionTemplate>::LeakedPersistent;
std::map<std::string, FunctionTemplateHandle> function_templates_;

v8::Local<v8::Object> CreateObjectWithName(v8::Isolate* isolate,
                                           const std::string& name) {
  if (name == "Object")
    return v8::Object::New(isolate);

  if (ContainsKey(function_templates_, name))
    return v8::Local<v8::FunctionTemplate>::New(
        isolate, function_templates_[name])->GetFunction()->NewInstance();

  v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New(isolate);
  t->SetClassName(mate::StringToV8(isolate, name));
  function_templates_[name] = FunctionTemplateHandle(isolate, t);
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
