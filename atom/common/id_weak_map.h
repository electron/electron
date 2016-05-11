// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ID_WEAK_MAP_H_
#define ATOM_COMMON_ID_WEAK_MAP_H_

#include "atom/common/key_weak_map.h"

namespace atom {

// Provides key increments service in addition to KeyWeakMap.
class IDWeakMap : public KeyWeakMap<int32_t> {
 public:
  IDWeakMap();
  ~IDWeakMap() override;

  // Adds |object| to WeakMap and returns its allocated |id|.
  int32_t Add(v8::Isolate* isolate, v8::Local<v8::Object> object);

 private:
  // Returns next available ID.
  int32_t GetNextID();

  // ID of next stored object.
  int32_t next_id_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace atom

#endif  // ATOM_COMMON_ID_WEAK_MAP_H_
