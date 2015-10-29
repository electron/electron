// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_weak_map.h"

#include "atom/common/node_includes.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

WeakMap::WeakMap() {
  id_weak_map_.reset(new atom::IDWeakMap);
}

WeakMap::~WeakMap() {
}

void WeakMap::Set(v8::Isolate* isolate,
                  int32_t id,
                  v8::Local<v8::Object> object) {
  id_weak_map_->Set(isolate, id, object);
}

v8::Local<v8::Object> WeakMap::Get(v8::Isolate* isolate, int32_t id) {
  return id_weak_map_->Get(isolate, id).ToLocalChecked();
}

bool WeakMap::Has(int32_t id) {
  return id_weak_map_->Has(id);
}

void WeakMap::Remove(int32_t id) {
  id_weak_map_->Remove(id);
}

bool WeakMap::IsDestroyed() const {
  return !id_weak_map_;
}

// static
void WeakMap::BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("set", &WeakMap::Set)
      .SetMethod("get", &WeakMap::Get)
      .SetMethod("has", &WeakMap::Has)
      .SetMethod("remove", &WeakMap::Remove);
}

// static
mate::Wrappable* WeakMap::Create(v8::Isolate* isolate) {
  return new WeakMap;
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WeakMap;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<WeakMap>(
      isolate, "WeakMap", base::Bind(&WeakMap::Create));
  mate::Dictionary weak_map(isolate, constructor);
  mate::Dictionary dict(isolate, exports);
  dict.Set("WeakMap", weak_map);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_weak_map, Initialize)
