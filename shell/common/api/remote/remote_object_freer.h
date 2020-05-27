// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_API_REMOTE_REMOTE_OBJECT_FREER_H_
#define SHELL_COMMON_API_REMOTE_REMOTE_OBJECT_FREER_H_

#include <map>
#include <string>

#include "shell/common/api/object_life_monitor.h"

namespace electron {

class RemoteObjectFreer : public ObjectLifeMonitor {
 public:
  static void BindTo(v8::Isolate* isolate,
                     v8::Local<v8::Object> target,
                     const std::string& context_id,
                     int object_id);
  static void AddRef(const std::string& context_id, int object_id);

 protected:
  RemoteObjectFreer(v8::Isolate* isolate,
                    v8::Local<v8::Object> target,
                    const std::string& context_id,
                    int object_id);
  ~RemoteObjectFreer() override;

  void RunDestructor() override;

  // { context_id => { object_id => ref_count }}
  static std::map<std::string, std::map<int, int>> ref_mapper_;

 private:
  std::string context_id_;
  int object_id_;
  int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(RemoteObjectFreer);
};

}  // namespace electron

#endif  // SHELL_COMMON_API_REMOTE_REMOTE_OBJECT_FREER_H_
