// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTER_H_
#define SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTER_H_

#include <set>

#include "gin/converter.h"

namespace gin {

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

}  // namespace gin

#endif  // SHELL_COMMON_GIN_CONVERTERS_STD_CONVERTER_H_
