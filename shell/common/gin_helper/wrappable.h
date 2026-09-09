// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_

#include "base/functional/bind.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/per_context_template_data.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val);

}  // namespace internal

template <typename T>
class Wrappable : public WrappableBase {
 public:
  Wrappable() = default;

  template <typename Sig>
  static v8::Local<v8::FunctionTemplate> CreateConstructorTemplate(
      v8::Isolate* isolate,
      const base::RepeatingCallback<Sig>& constructor) {
    v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(&internal::InvokeNew<Sig>, constructor));
    templ->InstanceTemplate()->SetInternalFieldCount(1);
    T::BuildPrototype(isolate, templ);
    auto* data = PerContextTemplateData::From(isolate->GetCurrentContext(),
                                              &kTemplateKey);
    if (data)
      data->function_template.Reset(isolate, templ);
    return templ;
  }

  static v8::Local<v8::FunctionTemplate> GetConstructor(v8::Isolate* isolate) {
    // Fill the object template.
    auto* data = PerContextTemplateData::From(isolate->GetCurrentContext(),
                                              &kTemplateKey);
    v8::Local<v8::FunctionTemplate> templ;
    if (data)
      templ = data->function_template.Get(isolate);
    if (templ.IsEmpty()) {
      templ = v8::FunctionTemplate::New(isolate);
      templ->InstanceTemplate()->SetInternalFieldCount(1);
      T::BuildPrototype(isolate, templ);
      if (data)
        data->function_template.Reset(isolate, templ);
    }
    return templ;
  }

 protected:
  // Init the class with T::BuildPrototype.
  void Init(v8::Isolate* isolate) {
    v8::Local<v8::FunctionTemplate> templ = GetConstructor(isolate);

    // |wrapper| may be empty in some extreme cases, e.g., when
    // Object.prototype.constructor is overwritten.
    v8::Local<v8::Object> wrapper;
    if (!templ->InstanceTemplate()
             ->NewInstance(isolate->GetCurrentContext())
             .ToLocal(&wrapper)) {
      // The current wrappable object will be no longer managed by V8. Delete
      // this now.
      delete this;
      return;
    }
    InitWith(isolate, wrapper);
  }

 private:
  static inline const char kTemplateKey = 0;
};

}  // namespace gin_helper

namespace gin {

template <typename T>
struct Converter<
    T*,
    typename std::enable_if<
        std::is_convertible<T*, gin_helper::WrappableBase*>::value>::type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val)
      return val->GetWrapper();
    else
      return v8::Null(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<gin_helper::WrappableBase*>(
        gin_helper::internal::FromV8Impl(isolate, val)));
    return *out != nullptr;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
