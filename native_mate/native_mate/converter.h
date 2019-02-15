// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#ifndef NATIVE_MATE_CONVERTER_H_
#define NATIVE_MATE_CONVERTER_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "gin/converter.h"
#include "v8/include/v8.h"

namespace mate {

template <typename KeyType>
bool SetProperty(v8::Isolate* isolate,
                 v8::Local<v8::Object> object,
                 KeyType key,
                 v8::Local<v8::Value> value) {
  auto maybe = object->Set(isolate->GetCurrentContext(), key, value);
  return !maybe.IsNothing() && maybe.FromJust();
}

template <typename T>
struct ToV8ReturnsMaybe {
  static const bool value = false;
};

template <typename T, typename Enable = void>
struct Converter {};

template <>
struct Converter<void*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, void* val) {
    return v8::Undefined(isolate);
  }
};

template <>
struct Converter<std::nullptr_t> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, std::nullptr_t val) {
    return v8::Null(isolate);
  }
};

template <>
struct Converter<bool> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, bool val);
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val, bool* out);
};

#if !defined(OS_LINUX) && !defined(OS_FREEBSD)
template <>
struct Converter<unsigned long> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, unsigned long val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     unsigned long* out);
};
#endif

template <>
struct Converter<int32_t> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, int32_t val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     int32_t* out);
};

template <>
struct Converter<uint32_t> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, uint32_t val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     uint32_t* out);
};

template <>
struct Converter<int64_t> {
  // Warning: JavaScript cannot represent 64 integers precisely.
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, int64_t val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     int64_t* out);
};

template <>
struct Converter<uint64_t> {
  // Warning: JavaScript cannot represent 64 integers precisely.
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, uint64_t val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     uint64_t* out);
};

template <>
struct Converter<float> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, float val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     float* out);
};

template <>
struct Converter<double> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, double val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     double* out);
};

template <>
struct Converter<const char*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, const char* val);
};

template <>
struct Converter<base::StringPiece> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::StringPiece& val);
  // No conversion out is possible because StringPiece does not contain storage.
};

template <>
struct Converter<std::string> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::string& val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::string* out);
};

v8::Local<v8::String> StringToSymbol(v8::Isolate* isolate,
                                     const base::StringPiece& input);

template <>
struct Converter<v8::Local<v8::Function>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::Function> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Function>* out);
};

template <>
struct Converter<v8::Local<v8::Object>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::Object> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Object>* out);
};

template <>
struct Converter<v8::Local<v8::String>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::String> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::String>* out);
};

template <>
struct Converter<v8::Local<v8::External>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::External> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::External>* out);
};

template <>
struct Converter<v8::Local<v8::Array>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::Array> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Array>* out);
};

template <>
struct Converter<v8::Local<v8::Value>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Value>* out);
};

template <>
struct Converter<v8::Local<v8::Promise>> {
  static v8::Local<v8::Promise> ToV8(v8::Isolate* isolate,
                                     v8::Local<v8::Promise> val);
  // static bool FromV8(v8::Isolate* isolate,
  //                    v8::Local<v8::Value> val,
  //                    v8::Local<v8::Value>* out);
};

template <typename T>
struct Converter<std::vector<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::vector<T>& val) {
    v8::Local<v8::Array> result(
        v8::Array::New(isolate, static_cast<int>(val.size())));
    for (size_t i = 0; i < val.size(); ++i) {
      result->Set(static_cast<int>(i), Converter<T>::ToV8(isolate, val[i]));
    }
    return result;
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::vector<T>* out) {
    if (!val->IsArray())
      return false;

    std::vector<T> result;
    v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(val));
    uint32_t length = array->Length();
    for (uint32_t i = 0; i < length; ++i) {
      T item;
      if (!Converter<T>::FromV8(isolate, array->Get(i), &item))
        return false;
      result.push_back(item);
    }

    out->swap(result);
    return true;
  }
};

template <typename T>
struct Converter<std::set<T>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::set<T>& val) {
    v8::Local<v8::Array> result(
        v8::Array::New(isolate, static_cast<int>(val.size())));
    typename std::set<T>::const_iterator it;
    int i;
    for (i = 0, it = val.begin(); it != val.end(); ++it, ++i)
      result->Set(i, Converter<T>::ToV8(isolate, *it));
    return result;
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::set<T>* out) {
    if (!val->IsArray())
      return false;

    std::set<T> result;
    v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(val));
    uint32_t length = array->Length();
    for (uint32_t i = 0; i < length; ++i) {
      T item;
      if (!Converter<T>::FromV8(isolate, array->Get(i), &item))
        return false;
      result.insert(item);
    }

    out->swap(result);
    return true;
  }
};

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
      v8::Local<v8::Value> key = keys->Get(i);
      T value;
      if (Converter<T>::FromV8(isolate, dict->Get(key), &value))
        (*out)[gin::V8ToString(isolate, key)] = std::move(value);
    }
    return true;
  }
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::map<std::string, T>& val) {
    v8::Local<v8::Object> result = v8::Object::New(isolate);
    for (auto i = val.begin(); i != val.end(); i++) {
      result->Set(Converter<T>::ToV8(isolate, i->first),
                  Converter<T>::ToV8(isolate, i->second));
    }
    return result;
  }
};

// Convenience functions that deduce T.
template <typename T>
v8::Local<v8::Value> ConvertToV8(v8::Isolate* isolate, const T& input) {
  return Converter<T>::ToV8(isolate, input);
}

inline v8::Local<v8::Value> ConvertToV8(v8::Isolate* isolate,
                                        const char* input) {
  return Converter<const char*>::ToV8(isolate, input);
}

template <typename T>
v8::MaybeLocal<v8::Value> ConvertToV8(v8::Local<v8::Context> context,
                                      const T& input) {
  return Converter<T>::ToV8(context, input);
}

template <typename T, bool = ToV8ReturnsMaybe<T>::value>
struct ToV8Traits;

template <typename T>
struct ToV8Traits<T, true> {
  static bool TryConvertToV8(v8::Isolate* isolate,
                             const T& input,
                             v8::Local<v8::Value>* output) {
    auto maybe = ConvertToV8(isolate->GetCurrentContext(), input);
    if (maybe.IsEmpty())
      return false;
    *output = maybe.ToLocalChecked();
    return true;
  }
};

template <typename T>
struct ToV8Traits<T, false> {
  static bool TryConvertToV8(v8::Isolate* isolate,
                             const T& input,
                             v8::Local<v8::Value>* output) {
    *output = ConvertToV8(isolate, input);
    return true;
  }
};

template <typename T>
bool TryConvertToV8(v8::Isolate* isolate,
                    const T& input,
                    v8::Local<v8::Value>* output) {
  return ToV8Traits<T>::TryConvertToV8(isolate, input, output);
}

template <typename T>
bool ConvertFromV8(v8::Isolate* isolate,
                   v8::Local<v8::Value> input,
                   T* result) {
  return Converter<T>::FromV8(isolate, input, result);
}

inline v8::Local<v8::String> StringToV8(v8::Isolate* isolate,
                                        const base::StringPiece& input) {
  return ConvertToV8(isolate, input).As<v8::String>();
}

}  // namespace mate

#endif  // NATIVE_MATE_CONVERTER_H_
