// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_id_weak_map.h"

#include <algorithm>

#include "base/logging.h"
#include "native_mate/constructor.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap()
    : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
}

int32_t IDWeakMap::Add(v8::Isolate* isolate, v8::Local<v8::Object> object) {
  int32_t key = GetNextID();
  object->SetHiddenValue(mate::StringToV8(isolate, "IDWeakMapKey"),
                         mate::Converter<int32_t>::ToV8(isolate, key));

  map_[key] = new mate::RefCountedPersistent<v8::Object>(isolate, object);
  map_[key]->SetWeak(this, WeakCallback);
  return key;
}

v8::Local<v8::Value> IDWeakMap::Get(v8::Isolate* isolate, int32_t key) {
  if (!Has(key)) {
    node::ThrowError(isolate, "Invalid key");
    return v8::Undefined(isolate);
  }

  return map_[key]->NewHandle();
}

bool IDWeakMap::Has(int32_t key) const {
  return map_.find(key) != map_.end();
}

std::vector<int32_t> IDWeakMap::Keys() const {
  std::vector<int32_t> keys;
  keys.reserve(map_.size());
  for (auto it = map_.begin(); it != map_.end(); ++it)
    keys.push_back(it->first);
  return keys;
}

void IDWeakMap::Remove(int32_t key) {
  if (Has(key))
    map_.erase(key);
  else
    LOG(WARNING) << "Object with key " << key << " is being GCed for twice.";
}

int IDWeakMap::GetNextID() {
  return ++next_id_;
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

// static
void IDWeakMap::WeakCallback(
    const v8::WeakCallbackData<v8::Object, IDWeakMap>& data) {
  int32_t key = data.GetValue()->GetHiddenValue(
      mate::StringToV8(data.GetIsolate(), "IDWeakMapKey"))->Int32Value();
  data.GetParameter()->Remove(key);
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
  exports->Set(mate::StringToV8(isolate, "IDWeakMap"), constructor);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_id_weak_map, Initialize)
