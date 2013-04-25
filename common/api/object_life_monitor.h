// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012 Intel Corp. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_OBJECT_LIFE_MONITOR_H_
#define ATOM_COMMON_API_OBJECT_LIFE_MONITOR_H_

#include "base/basictypes.h"
#include "v8/include/v8.h"

namespace atom {

class ObjectLifeMonitor {
 public:
  static void BindTo(v8::Handle<v8::Object> target,
                     v8::Handle<v8::Value> destructor);

 private:
  ObjectLifeMonitor();
  virtual ~ObjectLifeMonitor();

  static void WeakCallback(v8::Isolate* isolate,
                           v8::Persistent<v8::Value> value,
                           void *data);

  v8::Persistent<v8::Object> handle_;

  DISALLOW_COPY_AND_ASSIGN(ObjectLifeMonitor);
};

}  // namespace atom

#endif  // ATOM_COMMON_API_OBJECT_LIFE_MONITOR_H_
