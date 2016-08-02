// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_WRAPPABLE_H_
#define NATIVE_MATE_WRAPPABLE_H_

#include "base/bind.h"
#include "native_mate/compat.h"
#include "native_mate/converter.h"
#include "native_mate/constructor.h"
#include "native_mate/template_util.h"

namespace mate {

namespace internal {

void* FromV8Impl(v8::Isolate* isolate, v8::Local<v8::Value> val);

}  // namespace internal

template<typename T>
class Wrappable : public WrappableBase {
 public:
  Wrappable() {}

  template<typename Sig>
  static void SetConstructor(v8::Isolate* isolate,
                             const std::string& name,
                             const base::Callback<Sig>& factory) {
    v8::Local<v8::FunctionTemplate> constructor = CreateFunctionTemplate(
        isolate, base::Bind(&internal::InvokeNew<Sig>, factory));
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(StringToV8(isolate, name));
    T::BuildPrototype(isolate, constructor->PrototypeTemplate());
    templ_ = new v8::Global<v8::FunctionTemplate>(isolate, constructor);
  }

  static v8::Local<v8::FunctionTemplate> GetConstructor(v8::Isolate* isolate) {
    // Fill the object template.
    if (!templ_) {
      v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate);
      templ->InstanceTemplate()->SetInternalFieldCount(1);
      T::BuildPrototype(isolate, templ->PrototypeTemplate());
      templ_ = new v8::Global<v8::FunctionTemplate>(isolate, templ);
    }
    return v8::Local<v8::FunctionTemplate>::New(isolate, *templ_);
  }

 protected:
  // Init the class with T::BuildPrototype.
  void Init(v8::Isolate* isolate) {
    v8::Local<v8::FunctionTemplate> templ = GetConstructor(isolate);

    // |wrapper| may be empty in some extreme cases, e.g., when
    // Object.prototype.constructor is overwritten.
    v8::Local<v8::Object> wrapper;
    if (!templ->InstanceTemplate()->NewInstance(
            isolate->GetCurrentContext()).ToLocal(&wrapper)) {
      // The current wrappable object will be no longer managed by V8. Delete
      // this now.
      delete this;
      return;
    }
    InitWith(isolate, wrapper);
  }

 private:
  static v8::Global<v8::FunctionTemplate>* templ_;  // Leaked on purpose

  DISALLOW_COPY_AND_ASSIGN(Wrappable);
};

// static
template<typename T>
v8::Global<v8::FunctionTemplate>* Wrappable<T>::templ_ = nullptr;

// This converter handles any subclass of Wrappable.
template <typename T>
struct Converter<T*,
                 typename std::enable_if<
                     std::is_convertible<T*, WrappableBase*>::value>::type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, T* val) {
    if (val)
      return val->GetWrapper();
    else
      return v8::Null(isolate);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, T** out) {
    *out = static_cast<T*>(static_cast<WrappableBase*>(
        internal::FromV8Impl(isolate, val)));
    return *out != nullptr;
  }
};

}  // namespace mate

#endif  // NATIVE_MATE_WRAPPABLE_H_
