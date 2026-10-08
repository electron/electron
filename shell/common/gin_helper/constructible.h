// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_

#include <concepts>
#include <memory>
#include <type_traits>

#include "gin/arguments.h"
#include "gin/per_isolate_data.h"
#include "gin/public/wrapper_info.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/event_emitter_template.h"
#include "shell/common/gin_helper/function_template_cache.h"
#include "shell/common/gin_helper/function_template_extensions.h"
#include "v8/include/v8-context.h"

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
//     static Example* New(...usual gin method arguments...);
//     static void FillObjectTemplate(
//         v8::Isolate*,
//         v8::Local<v8::ObjectTemplate>);
//   }
//
// Do NOT define the usual gin::Wrappable::GetObjectTemplateBuilder. It
// will not be called for Constructible classes.
//
// A class may also define
//     static void FillInstanceTemplate(v8::Isolate*,
//                                      v8::Local<v8::ObjectTemplate>);
// to put accessors on the instances themselves (own properties) rather than
// on the prototype.
//
// A C++ subclass of a Constructible class that is exposed as its own
// constructor may define
//     using ConstructibleParent = Base;
// so that its constructor inherits Base's, putting Base.prototype in the
// prototype chain of its instances. Base must itself be Constructible. Such a
// subclass does not derive from Constructible<Subclass>, call
// Constructible<Subclass>::GetConstructor() directly instead.
//
// To expose the constructor, call GetConstructor:
//
//   gin::Dictionary dict(isolate, exports);
//   dict.Set("Example", Example::GetConstructor(
//                           isolate, context, &Example::kWrapperInfo));
template <typename T>
class Constructible {
 public:
  static v8::Local<v8::Function> GetConstructor(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context) {
    return GetConstructor(isolate, context, &T::kWrapperInfo);
  }

  static v8::Local<v8::Function> GetConstructor(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context,
      const gin::WrapperInfo* const wrapper_info) {
    v8::Local<v8::FunctionTemplate> constructor =
        GetConstructorTemplate(isolate, context, wrapper_info);
    if (constructor.IsEmpty())
      return {};
    return constructor->GetFunction(context).ToLocalChecked();
  }

  static v8::Local<v8::FunctionTemplate> GetConstructorTemplate(
      v8::Isolate* const isolate,
      v8::Local<v8::Context> context,
      const gin::WrapperInfo* const wrapper_info) {
    v8::Local<v8::FunctionTemplate> cached =
        GetCachedFunctionTemplate(isolate, wrapper_info);
    if (!cached.IsEmpty())
      return cached;

    v8::Local<v8::FunctionTemplate> constructor =
        gin::CreateConstructorFunctionTemplate(isolate,
                                               base::BindRepeating(&T::New));
    if constexpr (requires { typename T::ConstructibleParent; }) {
      using Parent = typename T::ConstructibleParent;
      static_assert(std::derived_from<T, Parent>,
                    "ConstructibleParent must be a base class of T");
      v8::Local<v8::FunctionTemplate> parent =
          Constructible<Parent>::GetConstructorTemplate(isolate, context,
                                                        &Parent::kWrapperInfo);
      if (parent.IsEmpty())
        return {};
      constructor->Inherit(parent);
    } else if (std::is_base_of<EventEmitterMixin<T>, T>::value) {
      constructor->Inherit(
          gin_helper::internal::GetEventEmitterTemplate(isolate));
    }
    constructor->SetClassName(gin::StringToV8(isolate, T::GetClassName()));
    T::FillObjectTemplate(isolate, constructor->PrototypeTemplate());
    if constexpr (requires { &T::FillInstanceTemplate; }) {
      T::FillInstanceTemplate(isolate, constructor->InstanceTemplate());
    }

    if (auto* data = gin::PerIsolateData::From(isolate)) {
      data->SetObjectTemplate(wrapper_info, constructor->InstanceTemplate());
    }
    SetCachedFunctionTemplate(isolate, wrapper_info, constructor);

    return constructor;
  }
};

inline bool ThrowIfNotConstructCall(gin::Arguments* args) {
  if (args->IsConstructCall())
    return true;
  args->ThrowTypeError("Requires constructor call");
  return false;
}

// Makes |wrappable| wrap the object V8 created for this `new` call, rather
// than a fresh object from the class's template, so that instances of
// JavaScript subclasses keep the subclass prototype. Call it from T::New right
// after allocating |wrappable|.
inline void BindToConstructCall(gin::Arguments* args,
                                gin::WrappableBase* wrappable) {
  v8::Local<v8::Object> receiver;
  if (args->IsConstructCall() && args->GetHolder(&receiver))
    wrappable->SetWrapper(args->isolate(), receiver);
}

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONSTRUCTIBLE_H_
