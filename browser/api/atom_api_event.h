// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_API_EVENT_H_
#define ATOM_BROWSER_ATOM_API_EVENT_H_

#include "base/basictypes.h"
#include "base/string16.h"
#include "vendor/node/src/node_object_wrap.h"

namespace atom {

namespace api {

class Event : public node::ObjectWrap {
 public:
  virtual ~Event();

  // Create a V8 Event object.
  static v8::Handle<v8::Object> CreateV8Object();

  // Get JSON string of the event.returnValue from a Event object.
  static string16 GetReturnValue(v8::Handle<v8::Object> event);

  // Accessor to return handle_, this follows Google C++ Style.
  v8::Persistent<v8::Object>& handle() { return handle_; }

  // Whether event.preventDefault() is called.
  bool prevent_default() const { return prevent_default_; }

 protected:
  Event();

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);
  static v8::Handle<v8::Value> PreventDefault(const v8::Arguments &args);

  static v8::Persistent<v8::FunctionTemplate> constructor_template_;

  bool prevent_default_;

  DISALLOW_COPY_AND_ASSIGN(Event);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_API_EVENT_H_
