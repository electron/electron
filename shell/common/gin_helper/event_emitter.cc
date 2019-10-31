// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/event_emitter.h"

#include "content/public/browser/render_frame_host.h"
#include "shell/browser/api/event.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "ui/events/event_constants.h"

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

v8::Local<v8::Object> CreateEvent(v8::Isolate* isolate,
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

v8::Local<v8::Object> CreateEventFromFlags(v8::Isolate* isolate, int flags) {
  const int mouse_button_flags =
      (ui::EF_RIGHT_MOUSE_BUTTON | ui::EF_LEFT_MOUSE_BUTTON |
       ui::EF_MIDDLE_MOUSE_BUTTON | ui::EF_BACK_MOUSE_BUTTON |
       ui::EF_FORWARD_MOUSE_BUTTON);
  const int is_mouse_click = static_cast<bool>(flags & mouse_button_flags);
  Dictionary obj = gin::Dictionary::CreateEmpty(isolate);
  obj.Set("shiftKey", static_cast<bool>(flags & ui::EF_SHIFT_DOWN));
  obj.Set("ctrlKey", static_cast<bool>(flags & ui::EF_CONTROL_DOWN));
  obj.Set("altKey", static_cast<bool>(flags & ui::EF_ALT_DOWN));
  obj.Set("metaKey", static_cast<bool>(flags & ui::EF_COMMAND_DOWN));
  obj.Set("triggeredByAccelerator", !is_mouse_click);
  return obj.GetHandle();
}

v8::Local<v8::Object> CreateNativeEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> sender,
    content::RenderFrameHost* frame,
    electron::mojom::ElectronBrowser::MessageSyncCallback callback) {
  v8::Local<v8::Object> event;
  if (frame && callback) {
    gin::Handle<Event> native_event = Event::Create(isolate);
    native_event->SetCallback(std::move(callback));
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    // No need to create native event if we do not need to send reply.
    event = CreateEvent(isolate);
  }

  Dictionary dict(isolate, event);
  dict.Set("sender", sender);
  // Should always set frameId even when callback is null.
  if (frame)
    dict.Set("frameId", frame->GetRoutingID());
  return event;
}

}  // namespace internal

}  // namespace gin_helper
