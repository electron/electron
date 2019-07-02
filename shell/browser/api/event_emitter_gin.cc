// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// TODO(deermichel): remove _gin suffix after removing mate

#include "shell/browser/api/event_emitter_gin.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "gin/arguments.h"
#include "gin/dictionary.h"
#include "gin/object_template_builder.h"
#include "shell/browser/api/event_gin.h"
#include "shell/common/node_includes.h"
#include "ui/events/event_constants.h"

namespace gin {

namespace {

v8::Persistent<v8::ObjectTemplate> event_template;

void PreventDefault(gin::Arguments* args) {
  gin::Dictionary self(args->isolate(), args->GetThis());
  self.Set("defaultPrevented", true);
}

// Create a pure JavaScript Event object.
v8::Local<v8::Object> CreateEventObject(v8::Isolate* isolate) {
  if (event_template.IsEmpty()) {
    event_template.Reset(
        isolate,
        ObjectTemplateBuilder(isolate, v8::ObjectTemplate::New(isolate))
            .SetMethod("preventDefault", &PreventDefault)
            .Build());
  }

  return v8::Local<v8::ObjectTemplate>::New(isolate, event_template)
      ->NewInstance(isolate->GetCurrentContext())
      .ToLocalChecked();
}

}  // namespace

namespace internal {

v8::Local<v8::Object> CreateJSEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Object> object,
    content::RenderFrameHost* sender,
    base::Optional<electron::mojom::ElectronBrowser::MessageSyncCallback>
        callback) {
  v8::Local<v8::Object> event;
  bool use_native_event = sender && callback;

  if (use_native_event) {
    gin::Handle<gin::Event> native_event = gin::Event::Create(isolate);
    native_event->SetSenderAndMessage(sender, std::move(callback));
    event = v8::Local<v8::Object>::Cast(native_event.ToV8());
  } else {
    event = CreateEventObject(isolate);
  }
  gin::Dictionary dict(isolate, event);
  dict.Set("sender", object);
  if (sender)
    dict.Set("frameId", sender->GetRoutingID());
  return event;
}

v8::Local<v8::Object> CreateCustomEvent(v8::Isolate* isolate,
                                        v8::Local<v8::Object> object,
                                        v8::Local<v8::Object> custom_event) {
  v8::Local<v8::Object> event = CreateEventObject(isolate);
  (void)event->SetPrototype(custom_event->CreationContext(), custom_event);
  gin::Dictionary(isolate, event).Set("sender", object);
  return event;
}

v8::Local<v8::Object> CreateEventFromFlags(v8::Isolate* isolate, int flags) {
  const int mouse_button_flags =
      (ui::EF_RIGHT_MOUSE_BUTTON | ui::EF_LEFT_MOUSE_BUTTON |
       ui::EF_MIDDLE_MOUSE_BUTTON | ui::EF_BACK_MOUSE_BUTTON |
       ui::EF_FORWARD_MOUSE_BUTTON);
  const int is_mouse_click = static_cast<bool>(flags & mouse_button_flags);
  // TODO(deermichel): does this work? correct?
  v8::Local<v8::Object> event = CreateEventObject(isolate);
  gin::Dictionary obj = gin::Dictionary(isolate, event);
  obj.Set("shiftKey", static_cast<bool>(flags & ui::EF_SHIFT_DOWN));
  obj.Set("ctrlKey", static_cast<bool>(flags & ui::EF_CONTROL_DOWN));
  obj.Set("altKey", static_cast<bool>(flags & ui::EF_ALT_DOWN));
  obj.Set("metaKey", static_cast<bool>(flags & ui::EF_COMMAND_DOWN));
  obj.Set("triggeredByAccelerator", !is_mouse_click);
  return event;
}

}  // namespace internal

}  // namespace gin
