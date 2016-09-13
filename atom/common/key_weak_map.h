// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_KEY_WEAK_MAP_H_
#define ATOM_COMMON_KEY_WEAK_MAP_H_

#include <unordered_map>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "v8/include/v8.h"

namespace atom {

// Like ES6's WeakMap, but the key is Integer and the value is Weak Pointer.
template<typename K>
class KeyWeakMap {
 public:
  // Records the key and self, used by SetWeak.
  struct KeyObject {
    K key;
    KeyWeakMap* self;
  };

  KeyWeakMap() {}
  virtual ~KeyWeakMap() {
    for (auto& p : map_)
      p.second.second.ClearWeak();
  }

  // Sets the object to WeakMap with the given |key|.
  void Set(v8::Isolate* isolate, const K& key, v8::Local<v8::Object> object) {
    KeyObject key_object = {key, this};
    auto& p = map_[key] =
        std::make_pair(key_object, v8::Global<v8::Object>(isolate, object));
    p.second.SetWeak(&(p.first), OnObjectGC, v8::WeakCallbackType::kParameter);
  }

  // Gets the object from WeakMap by its |key|.
  v8::MaybeLocal<v8::Object> Get(v8::Isolate* isolate, const K& key) {
    auto iter = map_.find(key);
    if (iter == map_.end())
      return v8::MaybeLocal<v8::Object>();
    else
      return v8::Local<v8::Object>::New(isolate, iter->second.second);
  }

  // Whethere there is an object with |key| in this WeakMap.
  bool Has(const K& key) const {
    return map_.find(key) != map_.end();
  }

  // Returns all objects.
  std::vector<v8::Local<v8::Object>> Values(v8::Isolate* isolate) const {
    std::vector<v8::Local<v8::Object>> keys;
    keys.reserve(map_.size());
    for (const auto& it : map_)
      keys.emplace_back(v8::Local<v8::Object>::New(isolate, it.second.second));
    return keys;
  }

  // Remove object with |key| in the WeakMap.
  void Remove(const K& key) {
    auto iter = map_.find(key);
    if (iter == map_.end())
      return;

    iter->second.second.ClearWeak();
    map_.erase(iter);
  }

 private:
  static void OnObjectGC(
      const v8::WeakCallbackInfo<typename KeyWeakMap<K>::KeyObject>& data) {
    KeyWeakMap<K>::KeyObject* key_object = data.GetParameter();
    key_object->self->Remove(key_object->key);
  }

  // Map of stored objects.
  std::unordered_map<
      K, std::pair<KeyObject, v8::Global<v8::Object>>> map_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // namespace atom

#endif  // ATOM_COMMON_KEY_WEAK_MAP_H_
