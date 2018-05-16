// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_

#include "atom/common/key_weak_map.h"
#include "base/threading/thread_task_runner_handle.h"
#include "native_mate/handle.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"

namespace atom {

namespace api {

template <typename K>
class KeyWeakMap : public mate::Wrappable<KeyWeakMap<K>>,
                   public KeyWeakMapObserver<K> {
 public:
  static mate::Handle<KeyWeakMap<K>> Create(v8::Isolate* isolate) {
    return mate::CreateHandle(isolate, new KeyWeakMap<K>(isolate));
  }

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
    prototype->SetClassName(mate::StringToV8(isolate, "KeyWeakMap"));
    mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
        .SetMethod("set", &KeyWeakMap<K>::Set)
        .SetMethod("get", &KeyWeakMap<K>::Get)
        .SetMethod("has", &KeyWeakMap<K>::Has)
        .SetMethod("remove", &KeyWeakMap<K>::Remove)
        .SetMethod("setCallback", &KeyWeakMap<K>::SetCallback);
  }

  // KeyWeakMapObserver:
  void OnRemoved(const K& key) override {
    auto handler = [](KeyWeakMap<K>* self, const K& key) {
      auto isolate = self->isolate();
      v8::HandleScope scope(isolate);

      auto callback = v8::Local<v8::Function>::New(isolate, self->callback_);
      if (callback.IsEmpty())
        return;

      v8::Local<v8::Value> args[] = {mate::ConvertToV8(isolate, key)};
      callback->Call(v8::Undefined(isolate), arraysize(args), args);
    };

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(handler, this, key));
  }

 protected:
  explicit KeyWeakMap(v8::Isolate* isolate) {
    mate::Wrappable<KeyWeakMap<K>>::Init(isolate);
    key_weak_map_.AddObserver(this);
  }
  ~KeyWeakMap() override { key_weak_map_.RemoveObserver(this); }

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

  void SetCallback(v8::Local<v8::Function> callback) {
    auto isolate = mate::Wrappable<KeyWeakMap<K>>::isolate();
    callback_ = v8::Global<v8::Function>(isolate, callback);
  }

  atom::KeyWeakMap<K> key_weak_map_;
  v8::Global<v8::Function> callback_;

  DISALLOW_COPY_AND_ASSIGN(KeyWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_KEY_WEAK_MAP_H_
