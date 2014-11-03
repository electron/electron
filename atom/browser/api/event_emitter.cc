// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"

#include "atom/browser/api/event.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "native_mate/arguments.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace mate {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(mate::Arguments* args) {
  args->GetThis()->Set(StringToV8(args->isolate(), "defaultPrevented"),
                       v8::True(args->isolate()));
}

// Create a pure JavaScript Event object.
v8::Local<v8::Object> CreateEventObject(v8::Isolate* isolate) {
  if (event_template.IsEmpty()) {
    event_template.Reset(isolate, ObjectTemplateBuilder(isolate)
        .SetMethod("preventDefault", &PreventDefault)
        .Build());
  }

  return v8::Local<v8::ObjectTemplate>::New(
      isolate, event_template)->NewInstance();
}

}  // namespace

EventEmitter::EventEmitter() {
}

bool EventEmitter::Emit(const base::StringPiece& name) {
  return Emit(name, base::ListValue());
}

bool EventEmitter::Emit(const base::StringPiece& name,
                        const base::ListValue& args) {
  return Emit(name, args, NULL, NULL);
}

bool EventEmitter::Emit(const base::StringPiece& name,
                        const base::ListValue& args,
                        content::WebContents* sender,
                        IPC::Message* message) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  v8::Handle<v8::Context> context = isolate->GetCurrentContext();
  scoped_ptr<atom::V8ValueConverter> converter(new atom::V8ValueConverter);

  // v8_args = [args...];
  Arguments v8_args;
  v8_args.reserve(args.GetSize());
  for (size_t i = 0; i < args.GetSize(); i++) {
    const base::Value* value(NULL);
    if (args.Get(i, &value))
      v8_args.push_back(converter->ToV8Value(value, context));
  }

  return Emit(isolate, name, v8_args, sender, message);
}

bool EventEmitter::Emit(v8::Isolate* isolate,
                        const base::StringPiece& name,
                        Arguments args,
                        content::WebContents* sender,
                        IPC::Message* message) {
  v8::Handle<v8::Object> event;
  bool use_native_event = sender && message;

  if (use_native_event) {
    mate::Handle<mate::Event> native_event = mate::Event::Create(isolate);
    native_event->SetSenderAndMessage(sender, message);
    event = v8::Handle<v8::Object>::Cast(native_event.ToV8());
  } else {
    event = CreateEventObject(isolate);
  }

  // args = [name, event, args...];
  args.insert(args.begin(), event);
  args.insert(args.begin(), mate::StringToV8(isolate, name));

  // this.emit.apply(this, args);
  node::MakeCallback(isolate, GetWrapper(isolate), "emit", args.size(),
                     &args[0]);

  return event->Get(StringToV8(isolate, "defaultPrevented"))->BooleanValue();
}

}  // namespace mate
