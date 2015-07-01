// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/id_weak_map.h"

#include <utility>

#include "native_mate/converter.h"

namespace atom {

IDWeakMap::IDWeakMap() : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
}

int32_t IDWeakMap::Add(v8::Isolate* isolate, v8::Local<v8::Object> object) {
  int32_t id = GetNextID();
  object->SetHiddenValue(mate::StringToSymbol(isolate, "IDWeakMapKey"),
                         mate::Converter<int32_t>::ToV8(isolate, id));

  auto global = make_linked_ptr(new v8::Global<v8::Object>(isolate, object));
  global->SetWeak(this, &WeakCallback);
  map_[id] = global;
  return id;
}

v8::MaybeLocal<v8::Object> IDWeakMap::Get(v8::Isolate* isolate, int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end())
    return v8::MaybeLocal<v8::Object>();
  else
    return v8::Local<v8::Object>::New(isolate, *iter->second);
}

bool IDWeakMap::Has(int32_t id) const {
  return map_.find(id) != map_.end();
}

std::vector<int32_t> IDWeakMap::Keys() const {
  std::vector<int32_t> keys;
  keys.reserve(map_.size());
  for (const auto& iter : map_)
    keys.emplace_back(iter.first);
  return keys;
}

std::vector<v8::Local<v8::Object>> IDWeakMap::Values(v8::Isolate* isolate) {
  std::vector<v8::Local<v8::Object>> keys;
  keys.reserve(map_.size());
  for (const auto& iter : map_)
    keys.emplace_back(v8::Local<v8::Object>::New(isolate, *iter.second));
  return keys;
}

void IDWeakMap::Remove(int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end())
    LOG(WARNING) << "Removing unexist object with ID " << id;
  else
    map_.erase(iter);
}

void IDWeakMap::Clear() {
  map_.clear();
}

int32_t IDWeakMap::GetNextID() {
  return ++next_id_;
}

// static
void IDWeakMap::WeakCallback(
    const v8::WeakCallbackData<v8::Object, IDWeakMap>& data) {
  int32_t id = data.GetValue()->GetHiddenValue(
      mate::StringToV8(data.GetIsolate(), "IDWeakMapKey"))->Int32Value();
  data.GetParameter()->Remove(id);
}

}  // namespace atom
