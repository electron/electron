// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
#define ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_

#include <string_view>
#include <utility>

#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter.h"

namespace gin_helper {

template <typename T>
class EventEmitterMixin {
 public:
  // disable copy
  EventEmitterMixin(const EventEmitterMixin&) = delete;
  EventEmitterMixin& operator=(const EventEmitterMixin&) = delete;

  // this.emit(name, new Event(), args...);
  // Returns true if event.preventDefault() was called during processing.
  template <typename... Args>
  bool Emit(const std::string_view name, Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    gin::Handle<internal::Event> event = internal::Event::New(isolate);
    gin_helper::EmitEvent(isolate, wrapper, name, event,
                          std::forward<Args>(args)...);
    return event->GetDefaultPrevented();
  }

  // this.emit(name, args...);
  template <typename... Args>
  void EmitWithoutEvent(const std::string_view name, Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return;
    gin_helper::EmitEvent(isolate, wrapper, name, std::forward<Args>(args)...);
  }

 protected:
  EventEmitterMixin() = default;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate) {
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
    auto* wrapper_info = &(static_cast<T*>(this)->kWrapperInfo);
    v8::Local<v8::FunctionTemplate> constructor =
        data->GetFunctionTemplate(wrapper_info);
    if (constructor.IsEmpty()) {
      constructor = v8::FunctionTemplate::New(isolate);
      constructor->SetClassName(
          gin::StringToV8(isolate, static_cast<T*>(this)->GetTypeName()));
      constructor->Inherit(internal::GetEventEmitterTemplate(isolate));
      data->SetFunctionTemplate(wrapper_info, constructor);
    }
    return gin::ObjectTemplateBuilder(isolate,
                                      static_cast<T*>(this)->GetTypeName(),
                                      constructor->InstanceTemplate());
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
