// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "atom/common/api/object_life_monitor.h"

namespace atom {

class RemoteObjectFreer : public ObjectLifeMonitor {
 public:
  static void BindTo(
      v8::Isolate* isolate, v8::Local<v8::Object> target, int object_id);

 protected:
  RemoteObjectFreer(
      v8::Isolate* isolate, v8::Local<v8::Object> target, int object_id);
  ~RemoteObjectFreer() override;

  void RunDestructor() override;

 private:
  int object_id_;

  DISALLOW_COPY_AND_ASSIGN(RemoteObjectFreer);
};

}  // namespace atom
