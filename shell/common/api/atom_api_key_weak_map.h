// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
#define SHELL_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/key_weak_map.h"

namespace electron {

namespace api {

template <typename K>
class KeyWeakMap : public mate::Wrappable<KeyWeakMap<K>> {
 public:
  static mate::Handle<KeyWeakMap<K>> Create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new KeyWeakMap<K>(isolate));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(gin::StringToV8(isolate, "KeyWeakMap"));
    gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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

  bool Has(const K& key) { return key_weak_map_.Has(key); }

  void Remove(const K& key) { key_weak_map_.Remove(key); }

  electron::KeyWeakMap<K> key_weak_map_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // namespace api

}  // namespace electron

namespace gin {

// TODO(zcbenz): Remove this after converting KeyWeakMap to gin::Wrapper.
template <typename T>
struct Converter<electron::api::KeyWeakMap<T>*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::KeyWeakMap<T>** out) {
    return mate::ConvertFromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::api::KeyWeakMap<T>* in) {
    return mate::ConvertToV8(isolate, in);
  }
};

}  // namespace gin

#endif  // SHELL_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
