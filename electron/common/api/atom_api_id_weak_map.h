// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
#define ELECTRON_COMMON_API_ATOM_API_ID_WEAK_MAP_H_

#include "electron/common/id_weak_map.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/handle.h"

namespace electron {

namespace api {

class IDWeakMap : public mate::Wrappable {
 public:
  static mate::Wrappable* Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  IDWeakMap();
  ~IDWeakMap();

 private:
  // Api for IDWeakMap.
  void Set(v8::Isolate* isolate, int32_t id, v8::Local<v8::Object> object);
  v8::Local<v8::Object> Get(v8::Isolate* isolate, int32_t id);
  bool Has(int32_t id);
  void Remove(int32_t id);
  void Clear();

  electron::IDWeakMap id_weak_map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
