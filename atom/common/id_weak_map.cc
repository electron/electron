// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/id_weak_map.h"

namespace atom {

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
