// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/event.h"

#include <utility>

#include "gin/object_template_builder.h"
#include "shell/common/gin_converters/blink_converter.h"

namespace gin_helper {

gin::WrapperInfo Event::kWrapperInfo = {gin::kEmbedderNativeGin};

Event::Event() {}

Event::~Event() = default;

void Event::SetCallback(InvokeCallback callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
}

void Event::PreventDefault(v8::Isolate* isolate) {
  v8::Local<v8::Object> self = GetWrapper(isolate).ToLocalChecked();
  self->Set(isolate->GetCurrentContext(),
            gin::StringToV8(isolate, "defaultPrevented"), v8::True(isolate))
      .Check();
}

bool Event::SendReply(v8::Isolate* isolate, v8::Local<v8::Value> result) {
  if (!callback_)
    return false;

  blink::CloneableMessage message;
  if (!gin::ConvertFromV8(isolate, result, &message)) {
    return false;
  }

  std::move(callback_).Run(std::move(message));
  return true;
}

gin::ObjectTemplateBuilder Event::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<Event>::GetObjectTemplateBuilder(isolate)
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetMethod("sendReply", &Event::SendReply);
}

const char* Event::GetTypeName() {
  return "WebRequest";
}

// static
gin::Handle<Event> Event::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new Event());
}

}  // namespace gin_helper
