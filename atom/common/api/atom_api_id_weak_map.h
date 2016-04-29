// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_

#include "atom/common/id_weak_map.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class IDWeakMap : public mate::Wrappable<IDWeakMap> {
 public:
  static mate::WrappableBase* Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit IDWeakMap(v8::Isolate* isolate);
  ~IDWeakMap();

 private:
  // Api for IDWeakMap.
  void Set(v8::Isolate* isolate, int32_t id, v8::Local<v8::Object> object);
  v8::Local<v8::Object> Get(v8::Isolate* isolate, int32_t id);
  bool Has(int32_t id);
  void Remove(int32_t id);
  void Clear();

  atom::IDWeakMap id_weak_map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
