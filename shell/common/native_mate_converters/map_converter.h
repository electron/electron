// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_
#define SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_

#include <map>
#include <string>
#include <utility>

#include "gin/converter.h"

namespace gin {

template <typename T>
struct Converter<std::map<std::string, T>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::map<std::string, T>* out) {
    if (!val->IsObject())
      return false;

    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::Object> dict = val->ToObject(context).ToLocalChecked();
    v8::Local<v8::Array> keys =
        dict->GetOwnPropertyNames(context).ToLocalChecked();
    for (uint32_t i = 0; i < keys->Length(); ++i) {
      v8::Local<v8::Value> key = keys->Get(context, i).ToLocalChecked();
      T value;
      if (Converter<T>::FromV8(
              isolate, dict->Get(context, key).ToLocalChecked(), &value))
        (*out)[gin::V8ToString(isolate, key)] = std::move(value);
    }
    return true;
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::map<std::string, T>& val) {
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    auto context = isolate->GetCurrentContext();
    for (auto i = val.begin(); i != val.end(); i++) {
      result
          ->Set(context, Converter<T>::ToV8(isolate, i->first),
                Converter<T>::ToV8(isolate, i->second))
          .Check();
    }
    return result;
  }
};

}  // namespace gin

namespace mate {

template <typename T>
struct Converter<std::map<std::string, T>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::map<std::string, T>* out) {
    return gin::Converter<std::map<std::string, T>>::FromV8(isolate, val, out);
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::map<std::string, T>& val) {
    return gin::Converter<std::map<std::string, T>>::ToV8(isolate, val);
  }
};

}  // namespace mate

#endif  // SHELL_COMMON_NATIVE_MATE_CONVERTERS_MAP_CONVERTER_H_
