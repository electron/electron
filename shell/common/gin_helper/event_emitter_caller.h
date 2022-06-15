// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_

#include <utility>
#include <vector>

#include "gin/converter.h"
#include "gin/wrappable.h"

namespace gin_helper {

namespace internal {

using ValueVector = std::vector<v8::Local<v8::Value>>;

v8::Local<v8::Value> CallMethodWithArgs(v8::Isolate* isolate,
                                        v8::Local<v8::Object> obj,
                                        const char* method,
                                        ValueVector* args);

}  // namespace internal

// obj.emit.apply(obj, name, args...);
// The caller is responsible of allocating a HandleScope.
template <typename StringType>
v8::Local<v8::Value> EmitEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> obj,
                               const StringType& name,
                               const internal::ValueVector& args) {
  internal::ValueVector concatenated_args = {gin::StringToV8(isolate, name)};
  concatenated_args.reserve(1 + args.size());
  concatenated_args.insert(concatenated_args.end(), args.begin(), args.end());
  return internal::CallMethodWithArgs(isolate, obj, "emit", &concatenated_args);
}

// obj.emit(name, args...);
// The caller is responsible of allocating a HandleScope.
template <typename StringType, typename... Args>
v8::Local<v8::Value> EmitEvent(v8::Isolate* isolate,
                               v8::Local<v8::Object> obj,
                               const StringType& name,
                               Args&&... args) {
  internal::ValueVector converted_args = {
      gin::StringToV8(isolate, name),
      gin::ConvertToV8(isolate, std::forward<Args>(args))...,
  };
  return internal::CallMethodWithArgs(isolate, obj, "emit", &converted_args);
}

// obj.custom_emit(args...)
template <typename... Args>
v8::Local<v8::Value> CustomEmit(v8::Isolate* isolate,
                                v8::Local<v8::Object> object,
                                const char* custom_emit,
                                Args&&... args) {
  internal::ValueVector converted_args = {
      gin::ConvertToV8(isolate, std::forward<Args>(args))...,
  };
  return internal::CallMethodWithArgs(isolate, object, custom_emit,
                                      &converted_args);
}

template <typename T, typename... Args>
v8::Local<v8::Value> CallMethod(v8::Isolate* isolate,
                                gin::Wrappable<T>* object,
                                const char* method_name,
                                Args&&... args) {
  v8::EscapableHandleScope scope(isolate);
  v8::Local<v8::Object> v8_object;
  if (object->GetWrapper(isolate).ToLocal(&v8_object))
    return scope.Escape(CustomEmit(isolate, v8_object, method_name,
                                   std::forward<Args>(args)...));
  else
    return v8::Local<v8::Value>();
}

template <typename T, typename... Args>
v8::Local<v8::Value> CallMethod(gin::Wrappable<T>* object,
                                const char* method_name,
                                Args&&... args) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  return CallMethod(isolate, object, method_name, std::forward<Args>(args)...);
}

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_EVENT_EMITTER_CALLER_H_
