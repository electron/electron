// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/ui_event.h"

#include "gin/dictionary.h"
#include "gin/handle.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/preventable_event.h"
#include "ui/events/event_constants.h"
#include "v8/include/v8.h"

namespace electron::api {

constexpr int mouse_button_flags =
    (ui::EF_RIGHT_MOUSE_BUTTON | ui::EF_LEFT_MOUSE_BUTTON |
     ui::EF_MIDDLE_MOUSE_BUTTON | ui::EF_BACK_MOUSE_BUTTON |
     ui::EF_FORWARD_MOUSE_BUTTON);

gin::Handle<gin_helper::internal::PreventableEvent> CreateEventFromFlags(
    int flags) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  const int is_mouse_click = static_cast<bool>(flags & mouse_button_flags);
  auto event = gin_helper::internal::PreventableEvent::New(isolate);
  gin::Dictionary dict(isolate, event.ToV8().As<v8::Object>());
  dict.Set("shiftKey", static_cast<bool>(flags & ui::EF_SHIFT_DOWN));
  dict.Set("ctrlKey", static_cast<bool>(flags & ui::EF_CONTROL_DOWN));
  dict.Set("altKey", static_cast<bool>(flags & ui::EF_ALT_DOWN));
  dict.Set("metaKey", static_cast<bool>(flags & ui::EF_COMMAND_DOWN));
  dict.Set("triggeredByAccelerator", !is_mouse_click);
  return event;
}

}  // namespace electron::api
