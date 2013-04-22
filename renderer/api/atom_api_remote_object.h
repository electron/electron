// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_API_REMOTE_OBJECT_H_
#define ATOM_RENDERER_ATOM_API_REMOTE_OBJECT_H_

#include <iosfwd>

#include "base/basictypes.h"
#include "vendor/node/src/node_object_wrap.h"

namespace atom {

namespace api {

// Every instance of RemoteObject represents an object in browser.
class RemoteObject : public node::ObjectWrap {
 public:
  virtual ~RemoteObject();

  static void Initialize(v8::Handle<v8::Object> target);

  // Tell browser to destroy this object.
  void Destroy();

  int id() const { return id_; }

 protected:
  explicit RemoteObject(v8::Handle<v8::Object> wrapper,
                        const std::string& type);
  explicit RemoteObject(v8::Handle<v8::Object> wrapper, int id);

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);
  static v8::Handle<v8::Value> Destroy(const v8::Arguments &args);

  int id_;

  DISALLOW_COPY_AND_ASSIGN(RemoteObject);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_API_REMOTE_OBJECT_H_
