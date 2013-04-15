// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_
#define ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "vendor/node/src/node_object_wrap.h"

namespace atom {

namespace api {

// Class interiting EventEmitter should assume it's a javascript object which
// interits require('events').EventEmitter, this class provides many helper
// methods to do event processing in C++.
class EventEmitter : public node::ObjectWrap {
 public:
  virtual ~EventEmitter();

  // Small accessor to return handle_, this follows Google C++ Style.
  v8::Persistent<v8::Object>& handle() { return handle_; }

 protected:
  explicit EventEmitter(v8::Handle<v8::Object> wrapper);

 private:
  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_
