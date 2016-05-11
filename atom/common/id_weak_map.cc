// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/id_weak_map.h"

#include <utility>

#include "native_mate/converter.h"

namespace atom {

namespace {

void OnObjectGC(const v8::WeakCallbackInfo<KeyWeakMap::KeyObject>& data) {
  KeyWeakMap::KeyObject* key_object = data.GetParameter();
  key_object->self->Remove(key_object->key);
}

}  // namespace

KeyWeakMap::KeyWeakMap() {
}

KeyWeakMap::~KeyWeakMap() {
  for (const auto& p : map_)
    p.second.second->ClearWeak();
}

void KeyWeakMap::Set(v8::Isolate* isolate,
                    int32_t key,
                    v8::Local<v8::Object> object) {
  auto value = make_linked_ptr(new v8::Global<v8::Object>(isolate, object));
  KeyObject key_object = {key, this};
  auto& p = map_[key] = std::make_pair(key_object, value);
  value->SetWeak(&(p.first), OnObjectGC, v8::WeakCallbackType::kParameter);
}

v8::MaybeLocal<v8::Object> KeyWeakMap::Get(v8::Isolate* isolate, int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end())
    return v8::MaybeLocal<v8::Object>();
  else
    return v8::Local<v8::Object>::New(isolate, *(iter->second.second));
}

bool KeyWeakMap::Has(int32_t id) const {
  return map_.find(id) != map_.end();
}

std::vector<v8::Local<v8::Object>> KeyWeakMap::Values(v8::Isolate* isolate) {
  std::vector<v8::Local<v8::Object>> keys;
  keys.reserve(map_.size());
  for (const auto& iter : map_) {
    const auto& value = *(iter.second.second);
    keys.emplace_back(v8::Local<v8::Object>::New(isolate, value));
  }
  return keys;
}

void KeyWeakMap::Remove(int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end()) {
    LOG(WARNING) << "Removing unexist object with ID " << id;
    return;
  }

  iter->second.second->ClearWeak();
  map_.erase(iter);
}

IDWeakMap::IDWeakMap() : next_id_(0) {
}

IDWeakMap::~IDWeakMap() {
}

int32_t IDWeakMap::Add(v8::Isolate* isolate, v8::Local<v8::Object> object) {
  int32_t id = GetNextID();
  Set(isolate, id, object);
  return id;
}

int32_t IDWeakMap::GetNextID() {
  return ++next_id_;
}

}  // namespace atom
