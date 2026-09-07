// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_

#include "gin/per_context_data.h"
#include "shell/common/gin_helper/event_emitter_template.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "shell/common/gin_helper/per_context_template_data.h"
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
    auto* const wrapper_info = &T::kWrapperInfo;
    auto* data = PerContextTemplateData::From(context, wrapper_info);
    v8::Local<v8::FunctionTemplate> constructor =
        data->function_template.Get(isolate);
    if (constructor.IsEmpty()) {
      if (!gin::CreateConstructorFunctionTemplate(isolate,
                                                  base::BindRepeating(&T::New))
               .ToLocal(&constructor)) {
        return {};
      }
      if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
        constructor->Inherit(
            gin_helper::internal::GetEventEmitterTemplate(isolate));
      }
      constructor->InstanceTemplate()->SetInternalFieldCount(
          gin::kNumberOfInternalFields);
      constructor->SetClassName(gin::StringToV8(isolate, T::GetClassName()));
      T::FillObjectTemplate(isolate, constructor->PrototypeTemplate());
      data->object_template.Reset(isolate, constructor->InstanceTemplate());
      data->function_template.Reset(isolate, constructor);
    }
    return constructor->GetFunction(context).ToLocalChecked();
  }

  static v8::Local<v8::Function> GetConstructor(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context,
      const gin::WrapperInfo* const wrapper_info) {
    gin::PerContextData* data = gin::PerContextData::From(context);
    CHECK(data);

    auto* constructor_data =
        PerContextTemplateData::From(context, wrapper_info);
    if (!constructor_data->function_template.IsEmpty()) {
      return constructor_data->function_template.Get(isolate)
          ->GetFunction(context)
          .ToLocalChecked();
    }

    v8::Local<v8::FunctionTemplate> constructor;
    if (!gin::CreateConstructorFunctionTemplate(isolate,
                                                base::BindRepeating(&T::New))
             .ToLocal(&constructor)) {
      return {};
    }
    if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
      constructor->Inherit(
          gin_helper::internal::GetEventEmitterTemplate(isolate));
    }
    constructor->InstanceTemplate()->SetInternalFieldCount(
        gin::kNumberOfInternalFields);
    constructor->SetClassName(gin::StringToV8(isolate, T::GetClassName()));
    T::FillObjectTemplate(isolate, constructor->PrototypeTemplate());

    data->SetObjectTemplate(wrapper_info, constructor->InstanceTemplate());
    constructor_data->function_template.Reset(isolate, constructor);

    return constructor->GetFunction(context).ToLocalChecked();
  }
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
