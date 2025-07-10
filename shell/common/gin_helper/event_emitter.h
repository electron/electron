// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_

#include <string_view>
#include <utility>

#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/wrappable.h"

namespace content {
class RenderFrameHost;
}

namespace gin_helper {

// Provide helpers to emit event in JavaScript.
template <typename T>
class EventEmitter : public gin_helper::Wrappable<T> {
 public:
  // this.emit(name, new Event(), args...);
  template <typename... Args>
  bool Emit(const std::string_view name, Args&&... args) {
    v8::Isolate* const isolate = this->isolate();
    v8::HandleScope handle_scope{isolate};
    v8::Local<v8::Object> wrapper = this->GetWrapper();
    if (wrapper.IsEmpty())
      return false;
    internal::Event* event = internal::Event::New(isolate);
    v8::Local<v8::Object> event_object =
        event->GetWrapper(isolate).ToLocalChecked();
    // It's possible that |this| will be deleted by EmitEvent, so save anything
    // we need from |this| before calling EmitEvent.
    EmitEvent(isolate, wrapper, name, event_object,
              std::forward<Args>(args)...);
    return event->GetDefaultPrevented();
  }

  // this.emit(name, args...);
  template <typename... Args>
  void EmitWithoutEvent(const std::string_view name, Args&&... args) {
    v8::Isolate* const isolate = this->isolate();
    v8::HandleScope handle_scope{isolate};
    v8::Local<v8::Object> wrapper = this->GetWrapper();
    if (wrapper.IsEmpty())
      return;
    EmitEvent(isolate, wrapper, name, std::forward<Args>(args)...);
  }

  // disable copy
  EventEmitter(const EventEmitter&) = delete;
  EventEmitter& operator=(const EventEmitter&) = delete;

 protected:
  EventEmitter() = default;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
