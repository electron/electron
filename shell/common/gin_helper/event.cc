// Copyright (c) 2023 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event.h"

#include "gin/object_template_builder.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"

namespace gin_helper::internal {

// static
Event* Event::New(v8::Isolate* isolate) {
  auto* event = cppgc::MakeGarbageCollected<Event>(
      isolate->GetCppHeap()->GetAllocationHandle());
  return event;
}
// static
v8::Local<v8::ObjectTemplate> Event::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetProperty("defaultPrevented", &Event::GetDefaultPrevented)
      .Build();
}

Event::Event() = default;

Event::~Event() = default;

gin::WrapperInfo Event::kWrapperInfo = {{gin::kEmbedderNativeGin},
                                        gin::kElectronEvent};

const gin::WrapperInfo* Event::wrapper_info() const {
  return &kWrapperInfo;
}

const char* Event::GetHumanReadableName() const {
  return "Electron / Event";
}

}  // namespace gin_helper::internal
