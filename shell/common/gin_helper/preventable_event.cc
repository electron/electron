// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/preventable_event.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"

namespace gin_helper::internal {

// static
gin::Handle<PreventableEvent> PreventableEvent::New(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender) {
  return gin::CreateHandle(isolate, new PreventableEvent(isolate, sender));
}
// static
v8::Local<v8::ObjectTemplate> PreventableEvent::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin::ObjectTemplateBuilder(isolate, "Event", templ)
      .SetMethod("preventDefault", &PreventableEvent::PreventDefault)
      .SetProperty("defaultPrevented", &PreventableEvent::GetDefaultPrevented)
      .SetProperty("sender", &PreventableEvent::GetSender)
      .Build();
}

PreventableEvent::PreventableEvent(v8::Isolate* isolate,
                                   v8::Local<v8::Object> sender)
    : sender_(isolate, sender) {}

PreventableEvent::~PreventableEvent() = default;

v8::Local<v8::Object> PreventableEvent::GetSender(v8::Isolate* isolate) {
  return sender_.Get(isolate);
}

gin::WrapperInfo PreventableEvent::kWrapperInfo = {gin::kEmbedderNativeGin};

gin::Handle<PreventableEvent> CreateCustomEvent(v8::Isolate* isolate,
                                                v8::Local<v8::Object> sender) {
  return PreventableEvent::New(isolate, sender);
}

}  // namespace gin_helper::internal
