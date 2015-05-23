// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_

#include <map>
#include <vector>

#include "base/basictypes.h"
#include "native_mate/scoped_persistent.h"
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
  int GetNextID();

  static void WeakCallback(
      const v8::WeakCallbackData<v8::Object, IDWeakMap>& data);

  int32_t next_id_;

  typedef scoped_refptr<mate::RefCountedPersistent<v8::Object> >
      RefCountedV8Object;
  std::map<int32_t, RefCountedV8Object> map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
