// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/api/electron_api_event_emitter.h"
#include "shell/browser/api/event.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/preventable_event.h"

namespace gin_helper::internal {

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
    event = CreateCustomEvent(isolate).ToV8().As<v8::Object>();
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
