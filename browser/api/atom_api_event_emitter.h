// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_
#define ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_

#include <iosfwd>

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

  // Same with Unwrap but if the handle is not a native object, which happened
  // when user inherits a native class, we would search for the native object,
  // otherwise we won't get the correct C++ object.
  template <typename T>
  static inline bool SafeUnwrap(v8::Handle<v8::Object> handle, T** pointer) {
    v8::Handle<v8::Object> real_handle = SearchNativeObject(handle);
    if (real_handle->InternalFieldCount() == 0)
      return false;

    *pointer = Unwrap<T>(real_handle);
    return true;
  }

  // Helper function to call "new" when constructor is called as normal
  // function, this usually happens when inheriting native class.
  //
  // This function also does some special hacks to make sure the inheritance
  // would work, because normal inheritance would not inherit internal fields.
  static v8::Handle<v8::Value> FromConstructorTemplate(
      v8::Persistent<v8::FunctionTemplate> t,
      const v8::Arguments& args);

  // Small accessor to return handle_, this follows Google C++ Style.
  v8::Persistent<v8::Object> handle() const { return handle_; }

 protected:
  explicit EventEmitter(v8::Handle<v8::Object> wrapper);

  static v8::Handle<v8::Object> SearchNativeObject(
      v8::Handle<v8::Object> handle);

 private:
  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EVENT_EMITTER_H_
