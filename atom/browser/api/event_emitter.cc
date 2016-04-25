// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/event_emitter.h"

#include "atom/browser/api/event.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

namespace mate {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(mate::Arguments* args) {
  mate::Dictionary self(args->isolate(), args->GetThis());
  self.Set("defaultPrevented", true);
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

namespace internal {

v8::Local<v8::Object> CreateJSEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    content::WebContents* sender,
    IPC::Message* message) {
  v8::Local<v8::Object> event;
  bool use_native_event = sender && message;

  if (use_native_event) {
    mate::Handle<mate::Event> native_event = mate::Event::Create(isolate);
    native_event->SetSenderAndMessage(sender, message);
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    event = CreateEventObject(isolate);
  }
  mate::Dictionary(isolate, event).Set("sender", object);
  return event;
}

v8::Local<v8::Object> CreateCustomEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    v8::Local<v8::Object> custom_event) {
  v8::Local<v8::Object> event = CreateEventObject(isolate);
  (void)event->SetPrototype(custom_event->CreationContext(), custom_event);
  mate::Dictionary(isolate, event).Set("sender", object);
  return event;
}

}  // namespace internal

}  // namespace mate
