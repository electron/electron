// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
#define ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_

#include <map>

#include "base/basictypes.h"
#include "common/v8/scoped_persistent.h"
#include "vendor/node/src/node_object_wrap.h"

namespace atom {

namespace api {

class IDWeakMap : public node::ObjectWrap {
 public:
  static void Initialize(v8::Handle<v8::Object> target);

 private:
  IDWeakMap();
  virtual ~IDWeakMap();

  bool Has(int key) const;
  void Erase(int key);
  int GetNextID();

  static void WeakCallback(v8::Isolate* isolate,
                           v8::Persistent<v8::Object>* value,
                           IDWeakMap* self);

  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Add(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Get(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Has(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Keys(const v8::FunctionCallbackInfo<v8::Value>& args);
  static void Remove(const v8::FunctionCallbackInfo<v8::Value>& args);

  int next_id_;
  std::map<int, RefCountedV8Object> map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_ID_WEAK_MAP_H_
