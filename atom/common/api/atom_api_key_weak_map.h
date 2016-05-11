// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_

#include "atom/common/key_weak_map.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

template<typename K>
class KeyWeakMap : public mate::Wrappable<KeyWeakMap<K>> {
 public:
  static mate::Handle<KeyWeakMap<K>> Create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new KeyWeakMap<K>(isolate));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype) {
    mate::ObjectTemplateBuilder(isolate, prototype)
        .SetMethod("set", &KeyWeakMap<K>::Set)
        .SetMethod("get", &KeyWeakMap<K>::Get)
        .SetMethod("has", &KeyWeakMap<K>::Has)
        .SetMethod("remove", &KeyWeakMap<K>::Remove);
  }

 protected:
  explicit KeyWeakMap(v8::Isolate* isolate) {
    mate::Wrappable<KeyWeakMap<K>>::Init(isolate);
  }
  ~KeyWeakMap() override {}

 private:
  // API for KeyWeakMap.
  void Set(v8::Isolate* isolate, const K& key, v8::Local<v8::Object> object) {
    key_weak_map_.Set(isolate, key, object);
  }

  v8::Local<v8::Object> Get(v8::Isolate* isolate, const K& key) {
    return key_weak_map_.Get(isolate, key).ToLocalChecked();
  }

  bool Has(const K& key) {
    return key_weak_map_.Has(key);
  }

  void Remove(const K& key) {
    key_weak_map_.Remove(key);
  }

  atom::KeyWeakMap<K> key_weak_map_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
