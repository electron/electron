// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_new.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"

namespace gin_helper::internal {

// static
gin::Handle<EventNew> EventNew::New(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new EventNew());
}
// static
v8::Local<v8::ObjectTemplate> EventNew::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin::ObjectTemplateBuilder(isolate, "Event", templ)
      .SetMethod("preventDefault", &EventNew::PreventDefault)
      .SetProperty("defaultPrevented", &EventNew::GetDefaultPrevented)
      .Build();
}

EventNew::EventNew() = default;

EventNew::~EventNew() = default;

gin::WrapperInfo EventNew::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // namespace gin_helper::internal
