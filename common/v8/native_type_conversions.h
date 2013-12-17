// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_V8_NATIVE_TYPE_CONVERSIONS_H_
#define ATOM_COMMON_V8_NATIVE_TYPE_CONVERSIONS_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "base/template_util.h"
#include "base/values.h"
#include "browser/api/atom_api_window.h"
#include "common/swap_or_assign.h"
#include "common/v8/scoped_persistent.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/renderer/v8_value_converter.h"
#include "ui/gfx/rect.h"
#include "url/gurl.h"

// Convert V8 value to arbitrary supported types.
struct FromV8Value {
  explicit FromV8Value(v8::Handle<v8::Value> value) : value_(value) {}

  operator int() {
    return value_->IntegerValue();
  }

  operator bool() {
    return value_->BooleanValue();
  }

  operator std::string() {
    return *v8::String::Utf8Value(value_);
  }

  operator string16() {
    v8::String::Value s(value_);
    return string16(reinterpret_cast<const char16*>(*s), s.length());
  }

  operator GURL() {
    std::string str = FromV8Value(value_);
    return GURL(str);
  }

  operator base::FilePath() {
    return base::FilePath::FromUTF8Unsafe(FromV8Value(value_));
  }

  operator gfx::Rect() {
    v8::Handle<v8::Object> rect = value_->ToObject();
    v8::Handle<v8::Value> x = rect->Get(v8::String::New("x"));
    v8::Handle<v8::Value> y = rect->Get(v8::String::New("y"));
    v8::Handle<v8::Value> width = rect->Get(v8::String::New("width"));
    v8::Handle<v8::Value> height = rect->Get(v8::String::New("height"));
    if (!x->IsNumber() || !y->IsNumber() ||
        !width->IsNumber() || !height->IsNumber())
      return gfx::Rect();
    else
      return gfx::Rect(x->IntegerValue(), y->IntegerValue(),
                       width->IntegerValue(), height->IntegerValue());
  }

  operator scoped_ptr<base::Value>() {
    scoped_ptr<content::V8ValueConverter> converter(
        new atom::V8ValueConverterImpl);
    return scoped_ptr<base::Value>(
        converter->FromV8Value(value_, v8::Context::GetCurrent()));
  }

  operator std::vector<std::string>() {
    std::vector<std::string> array;
    v8::Handle<v8::Array> v8_array = v8::Handle<v8::Array>::Cast(value_);
    for (uint32_t i = 0; i < v8_array->Length(); ++i)
      array.push_back(FromV8Value(v8_array->Get(i)));

    return array;
  }

  operator std::map<std::string, std::string>() {
    std::map<std::string, std::string> dict;
    v8::Handle<v8::Object> v8_dict = value_->ToObject();
    v8::Handle<v8::Array> v8_keys = v8_dict->GetOwnPropertyNames();
    for (uint32_t i = 0; i < v8_keys->Length(); ++i) {
      v8::Handle<v8::Value> v8_key = v8_keys->Get(i);
      std::string key = FromV8Value(v8_key);
      dict[key] = std::string(FromV8Value(v8_dict->Get(v8_key)));
    }

    return dict;
  }

  operator atom::NativeWindow*() {
    using atom::api::Window;
    if (value_->IsObject()) {
      Window* window = Window::Unwrap<Window>(value_->ToObject());
      if (window && window->window())
        return window->window();
    }
    return NULL;
  }

  operator atom::RefCountedV8Function() {
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value_);
    return atom::RefCountedV8Function(
        new atom::RefCountedPersistent<v8::Function>(func));
  }

  v8::Handle<v8::Value> value_;
};

// Convert arbitrary supported native type to V8 value.
inline v8::Handle<v8::Value> ToV8Value(int i) {
  return v8::Integer::New(i);
}

inline v8::Handle<v8::Value> ToV8Value(bool b) {
  return v8::Boolean::New(b);
}

inline v8::Handle<v8::Value> ToV8Value(const char* s) {
  return v8::String::New(s);
}

inline v8::Handle<v8::Value> ToV8Value(const std::string& s) {
  return v8::String::New(s.data(), s.size());
}

inline v8::Handle<v8::Value> ToV8Value(const string16& s) {
  return v8::String::New(reinterpret_cast<const uint16_t*>(s.data()), s.size());
}

inline v8::Handle<v8::Value> ToV8Value(const GURL& url) {
  return ToV8Value(url.spec());
}

inline v8::Handle<v8::Value> ToV8Value(const base::FilePath& path) {
  std::string path_string(path.AsUTF8Unsafe());
  return v8::String::New(path_string.data(), path_string.size());
}

inline v8::Handle<v8::Value> ToV8Value(void* whatever) {
  return v8::Undefined();
}

inline
v8::Handle<v8::Value> ToV8Value(const std::vector<base::FilePath>& paths) {
  v8::Handle<v8::Array> result = v8::Array::New(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
    result->Set(i, ToV8Value(paths[i]));
  return result;
}

// Check if a V8 Value is of specified type.
template<class T> inline
bool V8ValueCanBeConvertedTo(v8::Handle<v8::Value> value) {
  return false;
}

template<> inline
bool V8ValueCanBeConvertedTo<int>(v8::Handle<v8::Value> value) {
  return value->IsNumber();
}

template<> inline
bool V8ValueCanBeConvertedTo<bool>(v8::Handle<v8::Value> value) {
  return value->IsBoolean();
}

template<> inline
bool V8ValueCanBeConvertedTo<std::string>(v8::Handle<v8::Value> value) {
  return value->IsString();
}

template<> inline
bool V8ValueCanBeConvertedTo<string16>(v8::Handle<v8::Value> value) {
  return V8ValueCanBeConvertedTo<std::string>(value);
}

template<> inline
bool V8ValueCanBeConvertedTo<GURL>(v8::Handle<v8::Value> value) {
  return V8ValueCanBeConvertedTo<std::string>(value);
}

template<> inline
bool V8ValueCanBeConvertedTo<base::FilePath>(v8::Handle<v8::Value> value) {
  return V8ValueCanBeConvertedTo<std::string>(value);
}

template<> inline
bool V8ValueCanBeConvertedTo<gfx::Rect>(v8::Handle<v8::Value> value) {
  return value->IsObject();
}

template<> inline
bool V8ValueCanBeConvertedTo<scoped_ptr<base::Value>>(
    v8::Handle<v8::Value> value) {
  return value->IsObject();
}

template<> inline
bool V8ValueCanBeConvertedTo<std::vector<std::string>>(
    v8::Handle<v8::Value> value) {
  return value->IsArray();
}

template<> inline
bool V8ValueCanBeConvertedTo<std::map<std::string, std::string>>(
    v8::Handle<v8::Value> value) {
  return value->IsObject();
}

template<> inline
bool V8ValueCanBeConvertedTo<atom::NativeWindow*>(v8::Handle<v8::Value> value) {
  using atom::api::Window;
  if (value->IsObject()) {
    Window* window = Window::Unwrap<Window>(value->ToObject());
    if (window && window->window())
      return true;
  }
  return false;
}

template<> inline
bool V8ValueCanBeConvertedTo<atom::RefCountedV8Function>(
    v8::Handle<v8::Value> value) {
  return value->IsFunction();
}

// Check and convert V8's Arguments to native types.
template<typename T1> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* value,
                     int index = 0) {
  if (!V8ValueCanBeConvertedTo<T1>(args[index]))
    return false;
  internal::SwapOrAssign(*value,
                         FromV8Value(args[index]).operator T1());
  return true;
}

template<typename T1, typename T2> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2) {
  return FromV8Arguments<T1>(args, a1) && FromV8Arguments<T2>(args, a2, 1);
}

template<typename T1, typename T2, typename T3> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2,
                     T3* a3) {
  return FromV8Arguments<T1, T2>(args, a1, a2) &&
         FromV8Arguments<T3>(args, a3, 2);
}

template<typename T1, typename T2, typename T3, typename T4> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2,
                     T3* a3,
                     T4* a4) {
  return FromV8Arguments<T1, T2, T3>(args, a1, a2, a3) &&
         FromV8Arguments<T4>(args, a4, 3);
}

template<typename T1, typename T2, typename T3, typename T4, typename T5> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2,
                     T3* a3,
                     T4* a4,
                     T5* a5) {
  return FromV8Arguments<T1, T2, T3, T4>(args, a1, a2, a3, a4) &&
         FromV8Arguments<T5>(args, a5, 4);
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2,
                     T3* a3,
                     T4* a4,
                     T5* a5,
                     T6* a6) {
  return FromV8Arguments<T1, T2, T3, T4, T5>(args, a1, a2, a3, a4, a5) &&
         FromV8Arguments<T6>(args, a6, 5);
}

template<typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7> inline
bool FromV8Arguments(const v8::FunctionCallbackInfo<v8::Value>& args,
                     T1* a1,
                     T2* a2,
                     T3* a3,
                     T4* a4,
                     T5* a5,
                     T6* a6,
                     T7* a7) {
  return
      FromV8Arguments<T1, T2, T3, T4, T5, T6>(args, a1, a2, a3, a4, a5, a6) &&
      FromV8Arguments<T7>(args, a7, 6);
}

#endif  // ATOM_COMMON_V8_NATIVE_TYPE_CONVERSIONS_H_
