// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_EVENT_EMITTER_H_
#define ATOM_COMMON_API_ATOM_API_EVENT_EMITTER_H_

#include <string>

#include "base/basictypes.h"
#include "vendor/node/src/node_object_wrap.h"

namespace base {
class ListValue;
}

namespace atom {

namespace api {

// Class interiting EventEmitter should assume it's a javascript object which
// interits require('events').EventEmitter, this class provides many helper
// methods to do event processing in C++.
class EventEmitter : public node::ObjectWrap {
 public:
  virtual ~EventEmitter();

  // Emit an event and returns whether the handler has called preventDefault().
  bool Emit(const std::string& name);
  bool Emit(const std::string& name, base::ListValue* args);

 protected:
  explicit EventEmitter(v8::Handle<v8::Object> wrapper);

 private:
  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_EVENT_EMITTER_H_
