// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_

#include <string_view>
#include <utility>

#include "gin/handle.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/gin_helper/wrappable.h"

namespace content {
class RenderFrameHost;
}

namespace gin_helper {

// Provide helpers to emit event in JavaScript.
template <typename T>
class EventEmitter : public gin_helper::Wrappable<T> {
 public:
  using Base = gin_helper::Wrappable<T>;

  // Make the convenient methods visible:
  // https://isocpp.org/wiki/faq/templates#nondependent-name-lookup-members
  v8::Isolate* isolate() const { return Base::isolate(); }
  v8::Local<v8::Object> GetWrapper() const { return Base::GetWrapper(); }
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) const {
    return Base::GetWrapper(isolate);
  }

  // this.emit(name, new Event(), args...);
  template <typename... Args>
  bool Emit(const std::string_view name, Args&&... args) {
    v8::HandleScope handle_scope(isolate());
    v8::Local<v8::Object> wrapper = GetWrapper();
    if (wrapper.IsEmpty())
      return false;
    gin::Handle<gin_helper::internal::Event> event =
        internal::Event::New(isolate());
    return EmitWithEvent(name, event, std::forward<Args>(args)...);
  }

  // disable copy
  EventEmitter(const EventEmitter&) = delete;
  EventEmitter& operator=(const EventEmitter&) = delete;

 protected:
  EventEmitter() {}

 private:
  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitWithEvent(const std::string_view name,
                     gin::Handle<gin_helper::internal::Event> event,
                     Args&&... args) {
    // It's possible that |this| will be deleted by EmitEvent, so save anything
    // we need from |this| before calling EmitEvent.
    auto* isolate = this->isolate();
    gin_helper::EmitEvent(isolate, GetWrapper(), name, event,
                          std::forward<Args>(args)...);
    return event->GetDefaultPrevented();
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_H_
