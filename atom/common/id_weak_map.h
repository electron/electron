// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ID_WEAK_MAP_H_
#define ATOM_COMMON_ID_WEAK_MAP_H_

#include <unordered_map>
#include <vector>

#include "base/memory/linked_ptr.h"
#include "v8/include/v8.h"

namespace atom {

// Like ES6's WeakMap, but the key is Integer and the value is Weak Pointer.
class IDWeakMap {
 public:
  IDWeakMap();
  ~IDWeakMap();

  // Sets the object to WeakMap with the given |id|.
  void Set(v8::Isolate* isolate, int32_t id, v8::Local<v8::Object> object);

  // Adds |object| to WeakMap and returns its allocated |id|.
  int32_t Add(v8::Isolate* isolate, v8::Local<v8::Object> object);

  // Gets the object from WeakMap by its |id|.
  v8::MaybeLocal<v8::Object> Get(v8::Isolate* isolate, int32_t id);

  // Whethere there is an object with |id| in this WeakMap.
  bool Has(int32_t id) const;

  // Returns IDs of all available objects.
  std::vector<int32_t> Keys() const;

  // Returns all objects.
  std::vector<v8::Local<v8::Object>> Values(v8::Isolate* isolate);

  // Remove object with |id| in the WeakMap.
  void Remove(int32_t key);

  // Clears the weak map.
  void Clear();

 private:
  // Returns next available ID.
  int32_t GetNextID();

  // ID of next stored object.
  int32_t next_id_;

  // Map of stored objects.
  std::unordered_map<int32_t, linked_ptr<v8::Global<v8::Object>>> map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace atom

#endif  // ATOM_COMMON_ID_WEAK_MAP_H_
