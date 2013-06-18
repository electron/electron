// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_event_emitter.h"

#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "browser/api/atom_api_event.h"
#include "common/v8_value_converter_impl.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

EventEmitter::EventEmitter(v8::Handle<v8::Object> wrapper) {
  Wrap(wrapper);

  // process.emit('ATOM_BROWSER_INTERNAL_NEW', this).
  v8::Handle<v8::Value> args[] = {
      v8::String::New("ATOM_BROWSER_INTERNAL_NEW"),
      wrapper,
  };
  node::MakeCallback(node::process, "emit", 2, args);
}

EventEmitter::~EventEmitter() {
}

bool EventEmitter::Emit(const std::string& name) {
  base::ListValue args;
  return Emit(name, &args);
}

bool EventEmitter::Emit(const std::string& name, base::ListValue* args) {
  v8::HandleScope scope;

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();
  scoped_ptr<content::V8ValueConverter> converter(new V8ValueConverterImpl);

  v8::Handle<v8::Object> v8_event = Event::CreateV8Object();
  Event* event = Event::Unwrap<Event>(v8_event);

  // Generate arguments for calling handle.emit.
  std::vector<v8::Handle<v8::Value>> v8_args;
  v8_args.reserve(args->GetSize() + 2);
  v8_args.push_back(v8::String::New(name.c_str(), name.size()));
  v8_args.push_back(v8_event);
  for (size_t i = 0; i < args->GetSize(); i++) {
    base::Value* value = NULL;
    if (args->Get(i, &value)) {
      DCHECK(value);
      v8_args.push_back(converter->ToV8Value(value, context));
    } else {
      NOTREACHED() << "Wrong offset " << i << " for " << *args;
    }
  }

  node::MakeCallback(handle(), "emit", v8_args.size(), &v8_args[0]);

  bool prevent_default = event->prevent_default();

  // Don't wait for V8 GC, delete it immediately.
  delete event;

  return prevent_default;
}

// static
v8::Handle<v8::Value> EventEmitter::FromConstructorTemplate(
    v8::Persistent<v8::FunctionTemplate> t,
    const v8::Arguments& args) {
  v8::Handle<v8::Value> new_object = node::FromConstructorTemplate(t, args);
  args.This()->SetHiddenValue(v8::String::New("native_object"), new_object);
  return new_object;
}

// static
v8::Handle<v8::Object> EventEmitter::SearchNativeObject(
    v8::Handle<v8::Object> handle) {
  if (handle->InternalFieldCount() > 0)
    return handle;

  // If the object is not a native object, we assume the native object is
  // stored in it.
  return handle->GetHiddenValue(v8::String::New("native_object"))->ToObject();
}

}  // namespace api

}  // namespace atom
