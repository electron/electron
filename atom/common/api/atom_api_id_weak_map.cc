// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/common/api/atom_api_id_weak_map.h"

#include <algorithm>

#include "atom/common/v8/native_type_conversions.h"
#include "atom/common/v8/node_common.h"
#include "base/logging.h"
#include "native_mate/constructor.h"
#include "native_mate/function_template.h"
#include "native_mate/object_template_builder.h"

namespace atom {

namespace api {

IDWeakMap::IDWeakMap()
    : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
}

// static
void IDWeakMap::WeakCallback(v8::Isolate* isolate,
                             v8::Persistent<v8::Object>* value,
                             IDWeakMap* self) {
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> local = v8::Local<v8::Object>::New(isolate, *value);
  self->Remove(
      FromV8Value(local->GetHiddenValue(v8::String::New("IDWeakMapKey"))));
}

int32_t IDWeakMap::Add(v8::Isolate* isolate, v8::Handle<v8::Object> object) {
  int32_t key = GetNextID();
  object->SetHiddenValue(mate::StringToV8(isolate, "IDWeakMapKey"),
                         mate::Converter<int32_t>::ToV8(isolate, key));

  map_[key] = new RefCountedPersistent<v8::Object>(object);
  map_[key]->MakeWeak(this, WeakCallback);
  return key;
}

v8::Handle<v8::Value> IDWeakMap::Get(int32_t key) {
  if (!Has(key)) {
    node::ThrowError("Invalid key");
    return v8::Undefined();
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

void IDWeakMap::Initialize(v8::Handle<v8::Object> exports) {
  v8::Local<v8::FunctionTemplate> constructor = mate::CreateConstructor(
      v8::Isolate::GetCurrent(),
      "IDWeakMap",
      base::Bind(&mate::NewOperatorFactory<IDWeakMap>));

  mate::ObjectTemplateBuilder builder(
      v8::Isolate::GetCurrent(), constructor->PrototypeTemplate());
  builder.SetMethod("add", &IDWeakMap::Add)
         .SetMethod("get", &IDWeakMap::Get)
         .SetMethod("has", &IDWeakMap::Has)
         .SetMethod("keys", &IDWeakMap::Keys)
         .SetMethod("remove", &IDWeakMap::Remove);

  exports->Set(v8::String::NewSymbol("IDWeakMap"), constructor->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_common_id_weak_map, atom::api::IDWeakMap::Initialize)
