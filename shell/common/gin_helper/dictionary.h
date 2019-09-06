// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_HELPER_DICTIONARY_H_
#define SHELL_COMMON_GIN_HELPER_DICTIONARY_H_

#include <type_traits>

#include "base/bind.h"
#include "gin/dictionary.h"
#include "shell/common/gin_helper/function_template.h"

namespace gin_helper {

// Base template - used only for non-member function pointers. Other types
// either go to one of the below specializations, or go here and fail to compile
// because of base::Bind().
template <typename T, typename Enable = void>
struct CallbackTraits {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    return CreateFunctionTemplate(isolate, base::BindRepeating(callback));
  }
};

// Specialization for base::Callback.
template <typename T>
struct CallbackTraits<base::Callback<T>> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(
      v8::Isolate* isolate,
      const base::RepeatingCallback<T>& callback) {
    return CreateFunctionTemplate(isolate, callback);
  }
};

// Specialization for member function pointers. We need to handle this case
// specially because the first parameter for callbacks to MFP should typically
// come from the the JavaScript "this" object the function was called on, not
// from the first normal parameter.
template <typename T>
struct CallbackTraits<
    T,
    typename std::enable_if<std::is_member_function_pointer<T>::value>::type> {
  static v8::Local<v8::FunctionTemplate> CreateTemplate(v8::Isolate* isolate,
                                                        T callback) {
    int flags = HolderIsFirstArgument;
    return CreateFunctionTemplate(isolate, base::BindRepeating(callback),
                                  flags);
  }
};

// Adds a few more extends methods to gin::Dictionary.
//
// Note that as the destructor of gin::Dictionary is not virtual, and we want to
// convert between 2 types, we must not add any member.
class Dictionary : public gin::Dictionary {
 public:
  Dictionary(v8::Isolate* isolate, v8::Local<v8::Object> object)
      : gin::Dictionary(isolate, object) {}

  template <typename T>
  bool SetMethod(base::StringPiece key, const T& callback) {
    auto context = isolate()->GetCurrentContext();
    auto templ = CallbackTraits<T>::CreateTemplate(isolate(), callback);
    return GetHandle()
        ->Set(context, gin::StringToV8(isolate(), key),
              templ->GetFunction(context).ToLocalChecked())
        .ToChecked();
  }

  template <typename T>
  bool SetReadOnly(base::StringPiece key, const T& val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate()->GetCurrentContext(), gin::StringToV8(isolate(), key),
        v8_value, v8::ReadOnly);
    return !result.IsNothing() && result.FromJust();
  }

  bool Delete(base::StringPiece key) {
    v8::Maybe<bool> result = GetHandle()->Delete(
        isolate()->GetCurrentContext(), gin::StringToV8(isolate(), key));
    return !result.IsNothing() && result.FromJust();
  }

  v8::Local<v8::Object> GetHandle() const {
    return gin::ConvertToV8(isolate(),
                            *static_cast<const gin::Dictionary*>(this))
        .As<v8::Object>();
  }

 private:
  // DO NOT ADD ANY DATA MEMBER.
};

}  // namespace gin_helper

#endif  // SHELL_COMMON_GIN_HELPER_DICTIONARY_H_
