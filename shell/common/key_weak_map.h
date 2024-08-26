// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_KEY_WEAK_MAP_H_
#define ELECTRON_SHELL_COMMON_KEY_WEAK_MAP_H_

#include <unordered_map>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/memory/raw_ptr.h"
#include "v8/include/v8-forward.h"

namespace electron {

// Like ES6's WeakMap, with a K key and Weak Pointer value.
template <typename K>
class KeyWeakMap {
 public:
  KeyWeakMap() = default;
  ~KeyWeakMap() {
    for (auto& p : map_)
      p.second.value.ClearWeak();
  }

  // disable copy
  KeyWeakMap(const KeyWeakMap&) = delete;
  KeyWeakMap& operator=(const KeyWeakMap&) = delete;

  // Sets the object to WeakMap with the given |key|.
  void Set(v8::Isolate* isolate, const K& key, v8::Local<v8::Object> value) {
    auto& mapped = map_[key] =
        Mapped{this, key, v8::Global<v8::Object>(isolate, value)};
    mapped.value.SetWeak(&mapped, OnObjectGC, v8::WeakCallbackType::kParameter);
  }

  // Gets the object from WeakMap by its |key|.
  v8::MaybeLocal<v8::Object> Get(v8::Isolate* isolate, const K& key) {
    if (auto iter = map_.find(key); iter != map_.end())
      return v8::Local<v8::Object>::New(isolate, iter->second.value);
    return {};
  }

  // Returns all objects.
  std::vector<v8::Local<v8::Object>> Values(v8::Isolate* isolate) const {
    return base::ToVector(map_, [isolate](const auto& iter) {
      return v8::Local<v8::Object>::New(isolate, iter.second.value);
    });
  }

  // Remove object with |key| in the WeakMap.
  void Remove(const K& key) {
    if (auto item = map_.extract(key))
      item.mapped().value.ClearWeak();
  }

 private:
  // Records the key and self, used by SetWeak.
  struct Mapped {
    raw_ptr<KeyWeakMap> self;
    K key;
    v8::Global<v8::Object> value;
  };

  static void OnObjectGC(const v8::WeakCallbackInfo<Mapped>& data) {
    auto* mapped = data.GetParameter();
    mapped->self->Remove(mapped->key);
  }

  // Map of stored objects.
  std::unordered_map<K, Mapped> map_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_KEY_WEAK_MAP_H_
