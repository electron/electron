// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"

namespace gin_helper::internal {

// static
gin::Handle<Event> Event::New(v8::Isolate* isolate,
                              v8::Local<v8::Object> sender) {
  return gin::CreateHandle(isolate, new Event(isolate, sender));
}
// static
v8::Local<v8::ObjectTemplate> Event::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin::ObjectTemplateBuilder(isolate, "Event", templ)
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetProperty("defaultPrevented", &Event::GetDefaultPrevented)
      .SetProperty("sender", &Event::GetSender)
      .Build();
}

Event::Event(v8::Isolate* isolate, v8::Local<v8::Object> sender)
    : sender_(isolate, sender) {}

Event::~Event() = default;

v8::Local<v8::Object> Event::GetSender(v8::Isolate* isolate) {
  return sender_.Get(isolate);
}

gin::WrapperInfo Event::kWrapperInfo = {gin::kEmbedderNativeGin};

gin::Handle<Event> CreateEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> sender) {
  return Event::New(isolate, sender);
}

}  // namespace gin_helper::internal
