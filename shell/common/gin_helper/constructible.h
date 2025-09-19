// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_

#include "gin/per_isolate_data.h"
#include "shell/common/gin_helper/event_emitter_template.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "v8/include/v8-context.h"

namespace gin_helper {
template <typename T>
class EventEmitterMixin;

// Helper class for Wrappable objects which should be constructible with 'new'
// in JavaScript.
//
// To use, inherit from gin_helper::Wrappable and gin_helper::Constructible, and
// define the static methods New and FillObjectTemplate:
//
//   class Example : public gin_helper::DeprecatedWrappable<Example>,
//                   public gin_helper::Constructible<Example> {
//    public:
//     static gin_helper::Handle<Example> New(...usual gin method arguments...);
//     static void FillObjectTemplate(
//         v8::Isolate*,
//         v8::Local<v8::ObjectTemplate>);
//   }
//
// Do NOT define the usual gin_helper::Wrappable::GetObjectTemplateBuilder. It
// will not be called for Constructible classes.
//
// To expose the constructor, call GetConstructor:
//
//   gin::Dictionary dict(isolate, exports);
//   dict.Set("Example", Example::GetConstructor(context));
template <typename T>
class Constructible {
 public:
  static v8::Local<v8::Function> GetConstructor(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context) {
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
    auto* const wrapper_info = &T::kWrapperInfo;
    v8::Local<v8::FunctionTemplate> constructor =
        data->DeprecatedGetFunctionTemplate(wrapper_info);
    if (constructor.IsEmpty()) {
      constructor = gin::CreateConstructorFunctionTemplate(
          isolate, base::BindRepeating(&T::New));
      if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
        constructor->Inherit(
            gin_helper::internal::GetEventEmitterTemplate(isolate));
      }
      constructor->InstanceTemplate()->SetInternalFieldCount(
          gin::kNumberOfInternalFields);
      constructor->SetClassName(gin::StringToV8(isolate, T::GetClassName()));
      T::FillObjectTemplate(isolate, constructor->PrototypeTemplate());
      data->DeprecatedSetObjectTemplate(wrapper_info,
                                        constructor->InstanceTemplate());
      data->DeprecatedSetFunctionTemplate(wrapper_info, constructor);
    }
    return constructor->GetFunction(context).ToLocalChecked();
  }

  static v8::Local<v8::Function> GetConstructor(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context,
      const gin::WrapperInfo* const wrapper_info) {
    gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
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
      constructor->SetClassName(gin::StringToV8(isolate, T::GetClassName()));
      T::FillObjectTemplate(isolate, constructor->PrototypeTemplate());
      data->SetObjectTemplate(wrapper_info, constructor->InstanceTemplate());
      data->SetFunctionTemplate(wrapper_info, constructor);
    }
    return constructor->GetFunction(context).ToLocalChecked();
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
