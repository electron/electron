// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_

#include <vector>

#include "atom/common/id_weak_map.h"
#include "native_mate/wrappable.h"

namespace atom {

namespace api {

// Like ES6's WeakMap, but the key is Integer and the value is Weak Pointer.
class IDWeakMap : public mate::Wrappable {
 public:
  IDWeakMap();

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 private:
  virtual ~IDWeakMap();

  int32_t Add(v8::Isolate* isolate, v8::Local<v8::Object> object);
  v8::Local<v8::Value> Get(v8::Isolate* isolate, int32_t key);
  bool Has(int32_t key) const;
  std::vector<int32_t> Keys() const;
  void Remove(int32_t key);

  atom::IDWeakMap map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
