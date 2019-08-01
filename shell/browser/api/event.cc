// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/event.h"

#include <utility>

#include "native_mate/object_template_builder.h"
#include "shell/common/native_mate_converters/value_converter.h"

namespace mate {

Event::Event(v8::Isolate* isolate) {
  Init(isolate);
}

Event::~Event() {}

void Event::SetCallback(base::Optional<MessageSyncCallback> callback) {
  DCHECK(!callback_);
  callback_ = std::move(callback);
}

void Event::PreventDefault(v8::Isolate* isolate) {
  GetWrapper()
      ->Set(isolate->GetCurrentContext(),
            StringToV8(isolate, "defaultPrevented"), v8::True(isolate))
      .Check();
}

bool Event::SendReply(const base::Value& result) {
  if (!callback_)
    return false;

  std::move(*callback_).Run(result.Clone());
  callback_.reset();
  return true;
}

// static
Handle<Event> Event::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new Event(isolate));
}

// static
void Event::BuildPrototype(v8::Isolate* isolate,
                           v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Event"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("preventDefault", &Event::PreventDefault)
      .SetMethod("sendReply", &Event::SendReply);
}

}  // namespace mate
