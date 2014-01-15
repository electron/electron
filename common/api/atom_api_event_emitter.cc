// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/api/atom_api_event_emitter.h"

#include <vector>

#include "base/logging.h"
#include "browser/api/atom_api_event.h"
#include "common/v8/native_type_conversions.h"

#include "common/v8/node_common.h"

namespace atom {

namespace api {

EventEmitter::EventEmitter(v8::Handle<v8::Object> wrapper) {
  Wrap(wrapper);
}

EventEmitter::~EventEmitter() {
  // Clear the aligned pointer, it should have been done by ObjectWrap but
  // somehow node v0.11.x changed this behaviour.
  v8::HandleScope handle_scope(node_isolate);
  handle()->SetAlignedPointerInInternalField(0, NULL);
}

bool EventEmitter::Emit(const std::string& name) {
  base::ListValue args;
  return Emit(name, &args);
}

bool EventEmitter::Emit(const std::string& name, base::ListValue* args) {
  v8::HandleScope handle_scope(node_isolate);

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();
  scoped_ptr<V8ValueConverter> converter(new V8ValueConverter);

  v8::Handle<v8::Object> v8_event = Event::CreateV8Object();
  Event* event = Event::Unwrap<Event>(v8_event);

  // Generate arguments for calling handle.emit.
  std::vector<v8::Handle<v8::Value>> v8_args;
  v8_args.reserve(args->GetSize() + 2);
  v8_args.push_back(ToV8Value(name));
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

}  // namespace api

}  // namespace atom
