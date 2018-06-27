// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_OBJECT_TEMPLATE_BUILDER_H_
#define NATIVE_MATE_OBJECT_TEMPLATE_BUILDER_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/strings/string_piece.h"
#include "native_mate/converter.h"
#include "native_mate/function_template.h"
#include "v8/include/v8.h"

namespace mate {

namespace {

// Base template - used only for non-member function pointers. Other types
// either go to one of the below specializations, or go here and fail to compile
// because of base::Bind().
template<typename T, typename Enable = void>
struct CallbackTraits {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate, T callback) {
    return CreateFunctionTemplate(isolate, base::Bind(callback));
  }
};

// Specialization for base::Callback.
template<typename T>
struct CallbackTraits<base::Callback<T> > {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate, const base::Callback<T>& callback) {
    return CreateFunctionTemplate(isolate, callback);
  }
};

// Specialization for member function pointers. We need to handle this case
// specially because the first parameter for callbacks to MFP should typically
// come from the the JavaScript "this" object the function was called on, not
// from the first normal parameter.
template<typename T>
struct CallbackTraits<T, typename std::enable_if<
                           std::is_member_function_pointer<T>::value>::type> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate, T callback) {
    int flags = HolderIsFirstArgument;
    return CreateFunctionTemplate(isolate, base::Bind(callback), flags);
  }
};

// This specialization allows people to construct function templates directly if
// they need to do fancier stuff.
template<>
struct CallbackTraits<v8::Local<v8::FunctionTemplate> > {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Local<v8::FunctionTemplate> templ) {
    return templ;
  }
};

}  // namespace


// ObjectTemplateBuilder provides a handy interface to creating
// v8::ObjectTemplate instances with various sorts of properties.
class ObjectTemplateBuilder {
 public:
  explicit ObjectTemplateBuilder(
      v8::Isolate* isolate,
      v8::Local<v8::ObjectTemplate> templ);
  ~ObjectTemplateBuilder();

  // It's against Google C++ style to return a non-const ref, but we take some
  // poetic license here in order that all calls to Set() can be via the '.'
  // operator and line up nicely.
  template<typename T>
  ObjectTemplateBuilder& SetValue(const base::StringPiece& name, T val) {
    return SetImpl(name, ConvertToV8(isolate_, val));
  }

  // In the following methods, T and U can be function pointer, member function
  // pointer, base::Callback, or v8::FunctionTemplate. Most clients will want to
  // use one of the first two options. Also see mate::CreateFunctionTemplate()
  // for creating raw function templates.
  template<typename T>
  ObjectTemplateBuilder& SetMethod(const base::StringPiece& name,
                                   T callback) {
    return SetImpl(name,
                   CallbackTraits<T>::CreateTemplate(isolate_, callback));
  }
  template<typename T>
  ObjectTemplateBuilder& SetProperty(const base::StringPiece& name,
                                     T getter) {
    return SetPropertyImpl(
        name,
        CallbackTraits<T>::CreateTemplate(isolate_, getter),
        v8::Local<v8::FunctionTemplate>());
  }
  template<typename T, typename U>
  ObjectTemplateBuilder& SetProperty(const base::StringPiece& name,
                                     T getter,
                                     U setter) {
    return SetPropertyImpl(
        name,
        CallbackTraits<T>::CreateTemplate(isolate_, getter),
        CallbackTraits<U>::CreateTemplate(isolate_, setter));
  }

  // Add "destroy" and "isDestroyed" methods.
  ObjectTemplateBuilder& MakeDestroyable();

  v8::Local<v8::ObjectTemplate> Build();

 private:
  ObjectTemplateBuilder& SetImpl(const base::StringPiece& name,
                                 v8::Local<v8::Data> val);
  ObjectTemplateBuilder& SetPropertyImpl(
      const base::StringPiece& name, v8::Local<v8::FunctionTemplate> getter,
      v8::Local<v8::FunctionTemplate> setter);

  v8::Isolate* isolate_;

  // ObjectTemplateBuilder should only be used on the stack.
  v8::Local<v8::ObjectTemplate> template_;
};

}  // namespace mate

#endif  // NATIVE_MATE_OBJECT_TEMPLATE_BUILDER_H_
