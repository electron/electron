// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_id_weak_map.h"

#include "native_mate/constructor.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap() {
}

IDWeakMap::~IDWeakMap() {
}

int32_t IDWeakMap::Add(v8::Isolate* isolate, v8::Local<v8::Object> object) {
  return map_.Add(isolate, object);
}

v8::Local<v8::Value> IDWeakMap::Get(v8::Isolate* isolate, int32_t key) {
  v8::MaybeLocal<v8::Object> result = map_.Get(isolate, key);
  if (result.IsEmpty()) {
    isolate->ThrowException(v8::Exception::Error(
        mate::StringToV8(isolate, "Invalid key")));
    return v8::Undefined(isolate);
  } else {
    return result.ToLocalChecked();
  }
}

bool IDWeakMap::Has(int32_t key) const {
  return map_.Has(key);
}

std::vector<int32_t> IDWeakMap::Keys() const {
  return map_.Keys();
}

void IDWeakMap::Remove(int32_t key) {
  map_.Remove(key);
}

// static
void IDWeakMap::BuildPrototype(v8::Isolate* isolate,
                               v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("add", &IDWeakMap::Add)
      .SetMethod("get", &IDWeakMap::Get)
      .SetMethod("has", &IDWeakMap::Has)
      .SetMethod("keys", &IDWeakMap::Keys)
      .SetMethod("remove", &IDWeakMap::Remove);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  using atom::api::IDWeakMap;
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::Function> constructor = mate::CreateConstructor<IDWeakMap>(
      isolate,
      "IDWeakMap",
      base::Bind(&mate::NewOperatorFactory<IDWeakMap>));
  exports->Set(mate::StringToSymbol(isolate, "IDWeakMap"), constructor);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_id_weak_map, Initialize)
