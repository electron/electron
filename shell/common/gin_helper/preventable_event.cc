// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/preventable_event.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"

namespace gin_helper::internal {

// static
gin::Handle<PreventableEvent> PreventableEvent::New(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new PreventableEvent());
}
// static
void PreventableEvent::FillObjectTemplate(v8::Isolate* isolate,
                                          v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, "Event", templ)
      .SetMethod("preventDefault", &PreventableEvent::PreventDefault)
      .SetProperty("defaultPrevented", &PreventableEvent::GetDefaultPrevented);
}

PreventableEvent::~PreventableEvent() = default;

gin::WrapperInfo PreventableEvent::kWrapperInfo = {gin::kEmbedderNativeGin};

gin::Handle<PreventableEvent> CreateCustomEvent(v8::Isolate* isolate,
                                                v8::Local<v8::Object> sender) {
  auto event = PreventableEvent::New(isolate);
  if (!sender.IsEmpty())
    gin::Dictionary(isolate, event.ToV8().As<v8::Object>())
        .Set("sender", sender);
  return event;
}

}  // namespace gin_helper::internal
