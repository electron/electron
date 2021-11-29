// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
#define ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_

#include <utility>

#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/event_emitter.h"

namespace gin_helper {

namespace internal {
v8::Local<v8::FunctionTemplate> GetEventEmitterTemplate(v8::Isolate* isolate);
}  // namespace internal

template <typename T>
class EventEmitterMixin {
 public:
  // disable copy
  EventEmitterMixin(const EventEmitterMixin&) = delete;
  EventEmitterMixin& operator=(const EventEmitterMixin&) = delete;

  // this.emit(name, new Event(), args...);
  // Returns true if event.preventDefault() was called during processing.
  template <typename... Args>
  bool Emit(base::StringPiece name, Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    v8::Local<v8::Object> event = internal::CreateCustomEvent(isolate, wrapper);
    return EmitWithEvent(isolate, wrapper, name, event,
                         std::forward<Args>(args)...);
  }

  // this.emit(name, event, args...);
  template <typename... Args>
  bool EmitCustomEvent(base::StringPiece name,
                       v8::Local<v8::Object> custom_event,
                       Args&&... args) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> wrapper;
    if (!static_cast<T*>(this)->GetWrapper(isolate).ToLocal(&wrapper))
      return false;
    return EmitWithEvent(isolate, wrapper, name, custom_event,
                         std::forward<Args>(args)...);
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

 private:
  // this.emit(name, event, args...);
  template <typename... Args>
  static bool EmitWithEvent(v8::Isolate* isolate,
                            v8::Local<v8::Object> wrapper,
                            base::StringPiece name,
                            v8::Local<v8::Object> event,
                            Args&&... args) {
    auto context = isolate->GetCurrentContext();
    gin_helper::EmitEvent(isolate, wrapper, name, event,
                          std::forward<Args>(args)...);
    v8::Local<v8::Value> defaultPrevented;
    if (event->Get(context, gin::StringToV8(isolate, "defaultPrevented"))
            .ToLocal(&defaultPrevented)) {
      return defaultPrevented->BooleanValue(isolate);
    }
    return false;
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
