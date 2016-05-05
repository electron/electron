// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/string_map_converter.h"

namespace mate {

bool Converter<std::map<std::string, std::string>>::FromV8(v8::Isolate* isolate,
  v8::Local<v8::Value> val,
  std::map<std::string, std::string>* out) {
  if (!val->IsObject())
    return false;

  v8::Local<v8::Object> dict = val->ToObject();
  v8::Local<v8::Array> keys = dict->GetOwnPropertyNames();
  for (uint32_t i = 0; i < keys->Length(); ++i) {
    v8::Local<v8::Value> key = keys->Get(i);
    (*out)[V8ToString(key)] = V8ToString(dict->Get(key));
  }
  return true;
}

v8::Local<v8::Value> Converter<std::map<std::string, std::string>>::ToV8(
  v8::Isolate* isolate,
  const std::map<std::string, std::string>& in) {
  mate::Dictionary dict(isolate, v8::Object::New(isolate));

  for (auto const &pair : in) {
    dict.Set(pair.first, pair.second);
  }

  return dict.GetHandle();
}

}  // namespace mate
