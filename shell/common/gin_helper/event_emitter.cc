// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/event.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"

namespace gin_helper::internal {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(gin_helper::Arguments* args) {
  Dictionary self;
  if (args->GetHolder(&self))
    self.Set("defaultPrevented", true);
}

}  // namespace

v8::Local<v8::Object> CreateCustomEvent(v8::Isolate* isolate,
                                        v8::Local<v8::Object> sender,
                                        v8::Local<v8::Object> custom_event) {
  if (event_template.IsEmpty()) {
    event_template.Reset(
        isolate,
        ObjectTemplateBuilder(isolate, v8::ObjectTemplate::New(isolate))
            .SetMethod("preventDefault", &PreventDefault)
            .Build());
  }

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  CHECK(!context.IsEmpty());
  v8::Local<v8::Object> event =
      v8::Local<v8::ObjectTemplate>::New(isolate, event_template)
          ->NewInstance(context)
          .ToLocalChecked();
  if (!sender.IsEmpty())
    Dictionary(isolate, event).Set("sender", sender);
  if (!custom_event.IsEmpty())
    event->SetPrototype(context, custom_event).IsJust();
  return event;
}

v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronApiIPC::MessageSyncCallback callback) {
  v8::Local<v8::Object> event;
  if (frame && callback) {
    gin::Handle<Event> native_event = Event::Create(isolate);
    native_event->SetCallback(std::move(callback));
    event = native_event.ToV8().As<v8::Object>();
  } else {
    // No need to create native event if we do not need to send reply.
    event = CreateCustomEvent(isolate);
  }

  Dictionary dict(isolate, event);
  dict.Set("sender", sender);
  // Should always set frameId even when callback is null.
  if (frame) {
    dict.Set("frameId", frame->GetRoutingID());
    dict.Set("processId", frame->GetProcess()->GetID());
  }
  return event;
}

}  // namespace gin_helper::internal
