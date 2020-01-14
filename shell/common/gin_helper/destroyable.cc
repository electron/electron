// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/destroyable.h"

#include "gin/converter.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

namespace {

// Cached function templates, leaked on exit. (They are leaked in V8 anyway.)
v8::Global<v8::FunctionTemplate> g_destroy_func;
v8::Global<v8::FunctionTemplate> g_is_destroyed_func;

void DestroyFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();
  if (Destroyable::IsDestroyed(holder))
    return;

  // TODO(zcbenz): gin_helper::Wrappable will be removed.
  delete static_cast<gin_helper::WrappableBase*>(
      holder->GetAlignedPointerFromInternalField(0));
  holder->SetAlignedPointerInInternalField(0, nullptr);
}

void IsDestroyedFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(gin::ConvertToV8(
      info.GetIsolate(), Destroyable::IsDestroyed(info.Holder())));
}

}  // namespace

// static
bool Destroyable::IsDestroyed(v8::Local<v8::Object> object) {
  // An object is considered destroyed if it has no internal pointer or its
  // internal has been destroyed.
  return object->InternalFieldCount() == 0 ||
         object->GetAlignedPointerFromInternalField(0) == nullptr;
}

// static
void Destroyable::MakeDestroyable(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  // Cache the FunctionTemplate of "destroy" and "isDestroyed".
  if (g_destroy_func.IsEmpty()) {
    auto templ = v8::FunctionTemplate::New(isolate, DestroyFunc);
    templ->RemovePrototype();
    g_destroy_func.Reset(isolate, templ);
    templ = v8::FunctionTemplate::New(isolate, IsDestroyedFunc);
    templ->RemovePrototype();
    g_is_destroyed_func.Reset(isolate, templ);
  }

  auto proto_templ = prototype->PrototypeTemplate();
  proto_templ->Set(
      gin::StringToSymbol(isolate, "destroy"),
      v8::Local<v8::FunctionTemplate>::New(isolate, g_destroy_func));
  proto_templ->Set(
      gin::StringToSymbol(isolate, "isDestroyed"),
      v8::Local<v8::FunctionTemplate>::New(isolate, g_is_destroyed_func));
}

}  // namespace gin_helper
