// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// TODO(deermichel): remove _gin suffix after removing mate

#ifndef SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTERS_GIN_H_
#define SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTERS_GIN_H_

#include "gin/converter.h"

namespace gin {

// v8::Array
template <>
struct Converter<v8::Local<v8::Array>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::Array> val) {
    return val;
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Array>* out) {
    if (!val->IsArray())
      return false;
    *out = v8::Local<v8::Array>::Cast(val);
    return true;
  }
};

// std::set
template <typename T>
struct Converter<std::set<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::set<T>& val) {
    v8::Local<v8::Array> result(
        v8::Array::New(isolate, static_cast<int>(val.size())));
    auto context = isolate->GetCurrentContext();
    typename std::set<T>::const_iterator it;
    int i;
    for (i = 0, it = val.begin(); it != val.end(); ++it, ++i)
      result->Set(context, i, Converter<T>::ToV8(isolate, *it)).Check();
    return result;
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::set<T>* out) {
    if (!val->IsArray())
      return false;

    auto context = isolate->GetCurrentContext();
    std::set<T> result;
    v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(val));
    uint32_t length = array->Length();
    for (uint32_t i = 0; i < length; ++i) {
      T item;
      if (!Converter<T>::FromV8(isolate,
                                array->Get(context, i).ToLocalChecked(), &item))
        return false;
      result.insert(item);
    }

    out->swap(result);
    return true;
  }
};

// T&&
template <typename T>
v8::Local<v8::Value> ConvertToV8(v8::Isolate* isolate, T&& input) {
  return Converter<typename std::remove_reference<T>::type>::ToV8(
      isolate, std::move(input));
}

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTERS_GIN_H_
