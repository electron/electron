// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin/destroyable.h"

#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable_base.h"

namespace gin {

namespace {

// Cached function templates, leaked on exit. (They are leaked in V8 anyway.)
v8::Global<v8::FunctionTemplate> g_destroy_func;
v8::Global<v8::FunctionTemplate> g_is_destroyed_func;

// An object is considered destroyed if it has no internal pointer or its
// internal has been destroyed.
bool IsObjectDestroyed(v8::Local<v8::Object> holder) {
  return holder->InternalFieldCount() == 0 ||
         holder->GetAlignedPointerFromInternalField(0) == nullptr;
}

}  // namespace

// static
void Destroyable::Destroy(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Object> holder = info.Holder();
  if (IsObjectDestroyed(holder))
    return;

  // TODO(zcbenz): mate::Wrappable will be removed.
  delete static_cast<mate::WrappableBase*>(
      holder->GetAlignedPointerFromInternalField(0));
  holder->SetAlignedPointerInInternalField(0, nullptr);
}

// static
bool Destroyable::IsDestroyed(const v8::FunctionCallbackInfo<v8::Value>& info) {
  return IsObjectDestroyed(info.Holder());
}

// static
void Destroyable::MakeDestroyable(v8::Isolate* isolate,
                                  mate::ObjectTemplateBuilder* builder) {
  // Cache the FunctionTemplate of "destroy" and "isDestroyed".
  if (g_destroy_func.IsEmpty()) {
    auto templ = v8::FunctionTemplate::New(isolate, Destroy);
    templ->RemovePrototype();
    g_destroy_func.Reset(isolate, templ);
    templ = v8::FunctionTemplate::New(isolate, Destroy);
    templ->RemovePrototype();
    g_is_destroyed_func.Reset(isolate, templ);
  }

  builder->SetImpl(
      "destroy", v8::Local<v8::FunctionTemplate>::New(isolate, g_destroy_func));
  builder->SetImpl("isDestroyed", v8::Local<v8::FunctionTemplate>::New(
                                      isolate, g_is_destroyed_func));
}

}  // namespace gin
