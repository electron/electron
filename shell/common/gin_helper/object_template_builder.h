// Copyright (c) 2019 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_

#include <string_view>

#include "base/memory/raw_ptr.h"
#include "shell/common/gin_helper/function_template.h"

namespace gin_helper {

// This class works like gin::ObjectTemplateBuilder, but operates on existing
// prototype template instead of creating a new one.
//
// It also uses gin_helper::CreateFunctionTemplate for function templates to
// support gin_helper types.
//
// TODO(zcbenz): We should patch gin::ObjectTemplateBuilder to provide the same
// functionality after removing gin_helper/function_template.h.
class ObjectTemplateBuilder {
 public:
  ObjectTemplateBuilder(v8::Isolate* isolate,
                        v8::Local<v8::ObjectTemplate> templ);
  ~ObjectTemplateBuilder() = default;

  // It's against Google C++ style to return a non-const ref, but we take some
  // poetic license here in order that all calls to Set() can be via the '.'
  // operator and line up nicely.
  template <typename T>
  ObjectTemplateBuilder& SetValue(const std::string_view name, T val) {
    return SetImpl(name, ConvertToV8(isolate_, val));
  }

  // In the following methods, T and U can be function pointer, member function
  // pointer, base::RepeatingCallback, or v8::FunctionTemplate. Most clients
  // will want to use one of the first two options. Also see
  // gin::CreateFunctionTemplate() for creating raw function templates.
  template <typename T>
  ObjectTemplateBuilder& SetMethod(const std::string_view name,
                                   const T& callback) {
    return SetImpl(name, CallbackTraits<T>::CreateTemplate(isolate_, callback));
  }
  template <typename T>
  ObjectTemplateBuilder& SetProperty(const std::string_view name,
                                     const T& getter) {
    return SetPropertyImpl(name,
                           CallbackTraits<T>::CreateTemplate(isolate_, getter),
                           v8::Local<v8::FunctionTemplate>());
  }
  template <typename T, typename U>
  ObjectTemplateBuilder& SetProperty(const std::string_view name,
                                     const T& getter,
                                     const U& setter) {
    return SetPropertyImpl(name,
                           CallbackTraits<T>::CreateTemplate(isolate_, getter),
                           CallbackTraits<U>::CreateTemplate(isolate_, setter));
  }

  v8::Local<v8::ObjectTemplate> Build();

 private:
  ObjectTemplateBuilder& SetImpl(const std::string_view name,
                                 v8::Local<v8::Data> val);
  ObjectTemplateBuilder& SetPropertyImpl(
      const std::string_view name,
      v8::Local<v8::FunctionTemplate> getter,
      v8::Local<v8::FunctionTemplate> setter);

  raw_ptr<v8::Isolate> isolate_;

  // ObjectTemplateBuilder should only be used on the stack.
  v8::Local<v8::ObjectTemplate> template_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_OBJECT_TEMPLATE_BUILDER_H_
