// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_

#include "base/functional/bind.h"
#include "gin/per_isolate_data.h"
#include "gin/public/wrapper_info.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/wrappable_base.h"

namespace gin_helper {

bool IsValidWrappable(const v8::Local<v8::Value>& obj,
                      const gin::DeprecatedWrapperInfo* wrapper_info);

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val);
void* FromV8Impl(v8::Isolate* isolate,
                 v8::Local<v8::Value> val,
                 gin::DeprecatedWrapperInfo* info);

}  // namespace internal

template <typename T>
class Wrappable : public WrappableBase {
 public:
  Wrappable() = default;

  template <typename Sig>
  static void SetConstructor(v8::Isolate* isolate,
                             const base::RepeatingCallback<Sig>& constructor) {
    v8::Local<v8::FunctionTemplate> templ = gin_helper::CreateFunctionTemplate(
        isolate, base::BindRepeating(&internal::InvokeNew<Sig>, constructor));
    templ->InstanceTemplate()->SetInternalFieldCount(1);
    T::BuildPrototype(isolate, templ);
    gin::PerIsolateData::From(isolate)->DeprecatedSetFunctionTemplate(
        &kWrapperInfo, templ);
  }

  static v8::Local<v8::FunctionTemplate> GetConstructor(v8::Isolate* isolate) {
    // Fill the object template.
    auto* data = gin::PerIsolateData::From(isolate);
    auto templ = data->DeprecatedGetFunctionTemplate(&kWrapperInfo);
    if (templ.IsEmpty()) {
      templ = v8::FunctionTemplate::New(isolate);
      templ->InstanceTemplate()->SetInternalFieldCount(1);
      T::BuildPrototype(isolate, templ);
      data->DeprecatedSetFunctionTemplate(&kWrapperInfo, templ);
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
  static gin::DeprecatedWrapperInfo kWrapperInfo;
};

// Copied from https://chromium-review.googlesource.com/c/chromium/src/+/6799157
// Will be removed as part of https://github.com/electron/electron/issues/47922
template <typename T>
class DeprecatedWrappable : public DeprecatedWrappableBase {
 public:
  DeprecatedWrappable(const DeprecatedWrappable&) = delete;
  DeprecatedWrappable& operator=(const DeprecatedWrappable&) = delete;

  // Retrieve (or create) the v8 wrapper object corresponding to this object.
  v8::MaybeLocal<v8::Object> GetWrapper(v8::Isolate* isolate) {
    return GetWrapperImpl(isolate, &T::kWrapperInfo);
  }

 protected:
  DeprecatedWrappable() = default;
  ~DeprecatedWrappable() override = default;
};

// static
template <typename T>
gin::DeprecatedWrapperInfo Wrappable<T>::kWrapperInfo = {
    gin::kEmbedderNativeGin};

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

// This converter handles any subclass of DeprecatedWrappable.
template <typename T>
  requires(std::is_convertible_v<T*, gin_helper::DeprecatedWrappableBase*>)
struct Converter<T*> {
  static v8::MaybeLocal<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val == nullptr) {
      return v8::Null(isolate);
    }
    v8::Local<v8::Object> wrapper;
    if (!val->GetWrapper(isolate).ToLocal(&wrapper)) {
      return v8::MaybeLocal<v8::Value>();
    }
    return v8::MaybeLocal<v8::Value>(wrapper);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<gin_helper::DeprecatedWrappableBase*>(
        gin_helper::internal::FromV8Impl(isolate, val, &T::kWrapperInfo)));
    return *out != NULL;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_WRAPPABLE_H_
