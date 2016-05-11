// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/id_weak_map.h"

#include <utility>

#include "native_mate/converter.h"

namespace atom {

namespace {

struct ObjectKey {
  ObjectKey(int id, KeyWeakMap* map) : id(id), map(map) {}
  int id;
  KeyWeakMap* map;
};

void OnObjectGC(const v8::WeakCallbackInfo<ObjectKey>& data) {
  ObjectKey* key = data.GetParameter();
  key->map->Remove(key->id);
  delete key;
}

}  // namespace

KeyWeakMap::KeyWeakMap() {
}

KeyWeakMap::~KeyWeakMap() {
}

void KeyWeakMap::Set(v8::Isolate* isolate,
                    int32_t id,
                    v8::Local<v8::Object> object) {
  auto global = make_linked_ptr(new v8::Global<v8::Object>(isolate, object));
  ObjectKey* key = new ObjectKey(id, this);
  global->SetWeak(key, OnObjectGC, v8::WeakCallbackType::kParameter);
  map_[id] = global;
}

v8::MaybeLocal<v8::Object> KeyWeakMap::Get(v8::Isolate* isolate, int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end())
    return v8::MaybeLocal<v8::Object>();
  else
    return v8::Local<v8::Object>::New(isolate, *iter->second);
}

bool KeyWeakMap::Has(int32_t id) const {
  return map_.find(id) != map_.end();
}

std::vector<v8::Local<v8::Object>> KeyWeakMap::Values(v8::Isolate* isolate) {
  std::vector<v8::Local<v8::Object>> keys;
  keys.reserve(map_.size());
  for (const auto& iter : map_)
    keys.emplace_back(v8::Local<v8::Object>::New(isolate, *iter.second));
  return keys;
}

void KeyWeakMap::Remove(int32_t id) {
  auto iter = map_.find(id);
  if (iter == map_.end())
    LOG(WARNING) << "Removing unexist object with ID " << id;
  else
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
