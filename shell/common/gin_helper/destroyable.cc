// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/destroyable.h"

#include "base/no_destructor.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/wrappable_base.h"
#include "v8/include/v8-function.h"

namespace gin_helper {

namespace {

v8::Global<v8::FunctionTemplate>* GetDestroyFunc() {
  static base::NoDestructor<v8::Global<v8::FunctionTemplate>> destroy_func;
  return destroy_func.get();
}

v8::Global<v8::FunctionTemplate>* GetIsDestroyedFunc() {
  static base::NoDestructor<v8::Global<v8::FunctionTemplate>> is_destroyed_func;
  return is_destroyed_func.get();
}

void DestroyFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.This();
  if (Destroyable::IsDestroyed(holder))
    return;

  // TODO(zcbenz): gin_helper::Wrappable will be removed.
  delete static_cast<gin_helper::WrappableBase*>(
      holder->GetAlignedPointerFromInternalField(0));
  holder->SetAlignedPointerInInternalField(0, nullptr);
}

void IsDestroyedFunc(const v8::FunctionCallbackInfo<v8::Value>& info) {
  info.GetReturnValue().Set(gin::ConvertToV8(
      info.GetIsolate(), Destroyable::IsDestroyed(info.This())));
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
  if (GetDestroyFunc()->IsEmpty()) {
    auto templ = v8::FunctionTemplate::New(isolate, DestroyFunc);
    templ->RemovePrototype();
    GetDestroyFunc()->Reset(isolate, templ);
    templ = v8::FunctionTemplate::New(isolate, IsDestroyedFunc);
    templ->RemovePrototype();
    GetIsDestroyedFunc()->Reset(isolate, templ);
  }

  auto proto_templ = prototype->PrototypeTemplate();
  proto_templ->Set(
      gin::StringToSymbol(isolate, "destroy"),
      v8::Local<v8::FunctionTemplate>::New(isolate, *GetDestroyFunc()));
  proto_templ->Set(
      gin::StringToSymbol(isolate, "isDestroyed"),
      v8::Local<v8::FunctionTemplate>::New(isolate, *GetIsDestroyedFunc()));
}

}  // namespace gin_helper
