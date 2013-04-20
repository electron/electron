// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_H_
#define ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_H_

#include "base/basictypes.h"
#include "vendor/node/src/node_object_wrap.h"

namespace atom {

namespace api {

// Objects of this class will be recorded in C++ and available for RPC from
// renderer.
class RecordedObject : public node::ObjectWrap {
 public:
  virtual ~RecordedObject();

  // Small accessor to return handle_, this follows Google C++ Style.
  v8::Persistent<v8::Object>& handle() { return handle_; }

  int id() const { return id_; }

 protected:
  explicit RecordedObject(v8::Handle<v8::Object> wrapper);

 private:
  static v8::Handle<v8::Value> IDGetter(v8::Local<v8::String> property,
                                        const v8::AccessorInfo& info);

  int id_;

  DISALLOW_COPY_AND_ASSIGN(RecordedObject);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_RECORDED_OBJECT_H_
