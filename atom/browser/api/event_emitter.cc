// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"

#include <vector>

#include "atom/browser/api/event.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"

#include "atom/common/node_includes.h"

namespace mate {

EventEmitter::EventEmitter() {
}

bool EventEmitter::Emit(const base::StringPiece& name) {
  return Emit(name, base::ListValue());
}

bool EventEmitter::Emit(const base::StringPiece& name,
                        const base::ListValue& args) {
  v8::Locker locker(node_isolate);
  v8::HandleScope handle_scope(node_isolate);

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();
  scoped_ptr<atom::V8ValueConverter> converter(new atom::V8ValueConverter);

  mate::Handle<mate::Event> event = mate::Event::Create(node_isolate);

  // v8_args = [name, event, args...];
  std::vector<v8::Handle<v8::Value>> v8_args;
  v8_args.reserve(args.GetSize() + 2);
  v8_args.push_back(mate::StringToV8(node_isolate, name));
  v8_args.push_back(event.ToV8());
  for (size_t i = 0; i < args.GetSize(); i++) {
    const base::Value* value(NULL);
    if (args.Get(i, &value))
      v8_args.push_back(converter->ToV8Value(value, context));
  }

  // this.emit.apply(this, v8_args);
  node::MakeCallback(
      GetWrapper(node_isolate), "emit", v8_args.size(), &v8_args[0]);

  return event->prevent_default();
}

}  // namespace mate
