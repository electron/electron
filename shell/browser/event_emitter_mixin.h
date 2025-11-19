// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
#define ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_

#include <string_view>
#include <type_traits>
#include <utility>

#include "gin/object_template_builder.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/event.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "shell/common/gin_helper/handle.h"

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
    internal::Event* event = internal::Event::New(isolate);
    v8::Local<v8::Object> event_object =
        event->GetWrapper(isolate).ToLocalChecked();
    gin_helper::EmitEvent(isolate, wrapper, name, event_object,
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

    // DeprecatedWrapperInfo support will be removed as part of
    // https://github.com/electron/electron/issues/47922
    constexpr bool is_deprecated_wrapper =
        std::is_same_v<decltype(wrapper_info), gin::DeprecatedWrapperInfo*>;

    v8::Local<v8::FunctionTemplate> constructor;
    if constexpr (is_deprecated_wrapper) {
      constructor = data->DeprecatedGetFunctionTemplate(wrapper_info);
    } else {
      constructor = data->GetFunctionTemplate(wrapper_info);
    }

    const char* class_name = "";
    if constexpr (is_deprecated_wrapper) {
      class_name = static_cast<T*>(this)->GetTypeName();
    } else {
      class_name = static_cast<T*>(this)->GetClassName();
    }

    if (constructor.IsEmpty()) {
      constructor = v8::FunctionTemplate::New(isolate);
      constructor->SetClassName(gin::StringToV8(isolate, class_name));
      constructor->Inherit(internal::GetEventEmitterTemplate(isolate));
      if constexpr (is_deprecated_wrapper) {
        data->DeprecatedSetFunctionTemplate(wrapper_info, constructor);
      } else {
        data->SetFunctionTemplate(wrapper_info, constructor);
      }
    }
    return gin::ObjectTemplateBuilder(isolate, class_name,
                                      constructor->InstanceTemplate());
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_BROWSER_EVENT_EMITTER_MIXIN_H_
