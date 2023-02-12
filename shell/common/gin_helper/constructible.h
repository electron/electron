// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_

#include "gin/per_isolate_data.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gin_helper/function_template_extensions.h"

namespace gin_helper {
template <typename T>
class EventEmitterMixin;

// Helper class for Wrappable objects which should be constructible with 'new'
// in JavaScript.
//
// To use, inherit from gin::Wrappable and gin_helper::Constructible, and
// define the static methods New and FillObjectTemplate:
//
//   class Example : public gin::Wrappable<Example>,
//                   public gin_helper::Constructible<Example> {
//    public:
//     static gin::Handle<Tray> New(...usual gin method arguments...);
//     static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
//         v8::Isolate*,
//         v8::Local<v8::ObjectTemplate>);
//   }
//
// Do NOT define the usual gin::Wrappable::GetObjectTemplateBuilder. It will
// not be called for Constructible classes.
//
// To expose the constructor, call GetConstructor:
//
//   gin::Dictionary dict(isolate, exports);
//   dict.Set("Example", Example::GetConstructor(context));
template <typename T>
class Constructible {
 public:
  static v8::Local<v8::Function> GetConstructor(
      v8::Local<v8::Context> context) {
    v8::Isolate* isolate = context->GetIsolate();
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
    auto* wrapper_info = &T::kWrapperInfo;
    v8::Local<v8::FunctionTemplate> constructor =
        data->GetFunctionTemplate(wrapper_info);
    if (constructor.IsEmpty()) {
      constructor = gin::CreateConstructorFunctionTemplate(
          isolate, base::BindRepeating(&T::New));
      if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
        constructor->Inherit(
            gin_helper::internal::GetEventEmitterTemplate(isolate));
      }
      constructor->InstanceTemplate()->SetInternalFieldCount(
          gin::kNumberOfInternalFields);
      v8::Local<v8::ObjectTemplate> obj_templ =
          T::FillObjectTemplate(isolate, constructor->InstanceTemplate());
      data->SetObjectTemplate(wrapper_info, obj_templ);
      data->SetFunctionTemplate(wrapper_info, constructor);
    }
    return constructor->GetFunction(context).ToLocalChecked();
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
