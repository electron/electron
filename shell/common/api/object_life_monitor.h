// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_API_OBJECT_LIFE_MONITOR_H_
#define ELECTRON_SHELL_COMMON_API_OBJECT_LIFE_MONITOR_H_

#include "base/memory/weak_ptr.h"
#include "v8/include/v8.h"

namespace electron {

class ObjectLifeMonitor {
 protected:
  ObjectLifeMonitor(v8::Isolate* isolate, v8::Local<v8::Object> target);
  virtual ~ObjectLifeMonitor();

  // disable copy
  ObjectLifeMonitor(const ObjectLifeMonitor&) = delete;
  ObjectLifeMonitor& operator=(const ObjectLifeMonitor&) = delete;

  virtual void RunDestructor() = 0;

 private:
  static void OnObjectGC(const v8::WeakCallbackInfo<ObjectLifeMonitor>& data);
  static void Free(const v8::WeakCallbackInfo<ObjectLifeMonitor>& data);

  v8::Global<v8::Object> target_;

  base::WeakPtrFactory<ObjectLifeMonitor> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_API_OBJECT_LIFE_MONITOR_H_
