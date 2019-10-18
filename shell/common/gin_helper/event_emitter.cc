// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter.h"

#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

namespace gin_helper {

namespace internal {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(gin_helper::Arguments* args) {
  Dictionary self;
  if (args->GetHolder(&self))
    self.Set("defaultPrevented", true);
}

}  // namespace

v8::Local<v8::Object> CreateEventObject(v8::Isolate* isolate) {
  if (event_template.IsEmpty()) {
    event_template.Reset(
        isolate,
        ObjectTemplateBuilder(isolate, v8::ObjectTemplate::New(isolate))
            .SetMethod("preventDefault", &PreventDefault)
            .Build());
  }

  return v8::Local<v8::ObjectTemplate>::New(isolate, event_template)
      ->NewInstance(isolate->GetCurrentContext())
      .ToLocalChecked();
}

v8::Local<v8::Object> CreateCustomEvent(v8::Isolate* isolate,
                                        v8::Local<v8::Object> object,
                                        v8::Local<v8::Object> custom_event) {
  v8::Local<v8::Object> event = CreateEventObject(isolate);
  event->SetPrototype(custom_event->CreationContext(), custom_event).IsJust();
  Dictionary(isolate, event).Set("sender", object);
  return event;
}

}  // namespace internal

}  // namespace gin_helper
