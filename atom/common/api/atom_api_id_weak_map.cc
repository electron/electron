// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_id_weak_map.h"

#include "atom/common/node_includes.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap() {
}

IDWeakMap::~IDWeakMap() {
}

void IDWeakMap::Set(v8::Isolate* isolate,
                    int32_t id,
                    v8::Local<v8::Object> object) {
  id_weak_map_.Set(isolate, id, object);
}

v8::Local<v8::Object> IDWeakMap::Get(v8::Isolate* isolate, int32_t id) {
  return id_weak_map_.Get(isolate, id).ToLocalChecked();
}

bool IDWeakMap::Has(int32_t id) {
  return id_weak_map_.Has(id);
}

void IDWeakMap::Remove(int32_t id) {
  id_weak_map_.Remove(id);
}

void IDWeakMap::Clear() {
  id_weak_map_.Clear();
}

// static
void IDWeakMap::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("set", &IDWeakMap::Set)
      .SetMethod("get", &IDWeakMap::Get)
      .SetMethod("has", &IDWeakMap::Has)
      .SetMethod("remove", &IDWeakMap::Remove)
      .SetMethod("clear", &IDWeakMap::Clear);
}

// static
mate::Wrappable* IDWeakMap::Create(v8::Isolate* isolate) {
  return new IDWeakMap;
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::IDWeakMap;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<IDWeakMap>(
      isolate, "IDWeakMap", base::Bind(&IDWeakMap::Create));
  mate::Dictionary id_weak_map(isolate, constructor);
  mate::Dictionary dict(isolate, exports);
  dict.Set("IDWeakMap", id_weak_map);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_id_weak_map, Initialize)
