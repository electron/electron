// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_ID_WEAK_MAP_H_
#define ATOM_BROWSER_API_ATOM_API_ID_WEAK_MAP_H_

#include <map>

#include "base/basictypes.h"
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
  void Erase(v8::Isolate* isolate, int key);
  int GetNextID();

  static void WeakCallback(v8::Isolate* isolate,
                           v8::Persistent<v8::Value> value,
                           void *data);

  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> Add(const v8::Arguments& args);
  static v8::Handle<v8::Value> Get(const v8::Arguments& args);
  static v8::Handle<v8::Value> Has(const v8::Arguments& args);
  static v8::Handle<v8::Value> Keys(const v8::Arguments& args);
  static v8::Handle<v8::Value> Remove(const v8::Arguments& args);

  int next_id_;

  std::map<int, v8::Persistent<v8::Value>> map_;

  DISALLOW_COPY_AND_ASSIGN(IDWeakMap);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_ID_WEAK_MAP_H_
