// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMMON_V8_CONVERSIONS_H_
#define COMMON_V8_CONVERSIONS_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "browser/api/atom_api_window.h"
#include "v8/include/v8.h"

// Trick to overload functions by return value's type.
struct FromV8Value {
  explicit FromV8Value(v8::Handle<v8::Value> value) : value_(value) {}

  operator std::string() {
    return *v8::String::Utf8Value(value_);
  }

  operator int() {
    return value_->IntegerValue();
  }

  operator base::FilePath() {
    return base::FilePath::FromUTF8Unsafe(FromV8Value(value_));
  }

  operator std::vector<std::string>() {
    std::vector<std::string> array;
    v8::Handle<v8::Array> v8_array = v8::Handle<v8::Array>::Cast(value_);
    for (uint32_t i = 0; i < v8_array->Length(); ++i)
      array.push_back(FromV8Value(v8_array->Get(i)));

    return array;
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

  operator v8::Persistent<v8::Function>() {
    return value_->IsFunction() ?
      v8::Persistent<v8::Function>::New(
          node::node_isolate,
          v8::Handle<v8::Function>::Cast(value_)) :
      v8::Persistent<v8::Function>();
  }

  v8::Handle<v8::Value> value_;
};

inline v8::Handle<v8::Value> ToV8Value(const base::FilePath& path) {
  std::string path_string(path.AsUTF8Unsafe());
  return v8::String::New(path_string.data(), path_string.size());
}

inline v8::Handle<v8::Value> ToV8Value(void* whatever) {
  return v8::Undefined();
}

inline v8::Handle<v8::Value> ToV8Value(int code) {
  return v8::Integer::New(code);
}

inline
v8::Handle<v8::Value> ToV8Value(const std::vector<base::FilePath>& paths) {
  v8::Handle<v8::Array> result = v8::Array::New(paths.size());
  for (size_t i = 0; i < paths.size(); ++i)
    result->Set(i, ToV8Value(paths[i]));
  return result;
}

#endif  // COMMON_V8_CONVERSIONS_H_
