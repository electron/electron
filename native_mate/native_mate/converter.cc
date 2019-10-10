// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE.chromium file.

#include "native_mate/converter.h"

#include "v8/include/v8.h"

namespace mate {

namespace {

template <typename T, typename U>
bool FromMaybe(v8::Maybe<T> maybe, U* out) {
  if (maybe.IsNothing())
    return false;
  *out = static_cast<U>(maybe.FromJust());
  return true;
}

}  // namespace

v8::Local<v8::Value> Converter<bool>::ToV8(v8::Isolate* isolate, bool val) {
  return v8::Boolean::New(isolate, val);
}

bool Converter<bool>::FromV8(v8::Isolate* isolate,
                             v8::Local<v8::Value> val,
                             bool* out) {
  if (!val->IsBoolean())
    return false;
  *out = val.As<v8::Boolean>()->Value();
  return true;
}

#if !defined(OS_LINUX) && !defined(OS_FREEBSD)
v8::Local<v8::Value> Converter<unsigned long>::ToV8(  // NOLINT(runtime/int)
    v8::Isolate* isolate,
    unsigned long val) {  // NOLINT(runtime/int)
  return v8::Integer::New(isolate, val);
}

bool Converter<unsigned long>::FromV8(  // NOLINT(runtime/int)
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    unsigned long* out) {  // NOLINT(runtime/int)
  if (!val->IsNumber())
    return false;
  return FromMaybe(val->IntegerValue(isolate->GetCurrentContext()), out);
}
#endif

v8::Local<v8::Value> Converter<int32_t>::ToV8(v8::Isolate* isolate,
                                              int32_t val) {
  return v8::Integer::New(isolate, val);
}

bool Converter<int32_t>::FromV8(v8::Isolate* isolate,
                                v8::Local<v8::Value> val,
                                int32_t* out) {
  if (!val->IsInt32())
    return false;
  *out = val.As<v8::Int32>()->Value();
  return true;
}

v8::Local<v8::Value> Converter<uint32_t>::ToV8(v8::Isolate* isolate,
                                               uint32_t val) {
  return v8::Integer::NewFromUnsigned(isolate, val);
}

bool Converter<uint32_t>::FromV8(v8::Isolate* isolate,
                                 v8::Local<v8::Value> val,
                                 uint32_t* out) {
  if (!val->IsUint32())
    return false;
  *out = val.As<v8::Uint32>()->Value();
  return true;
}

v8::Local<v8::Value> Converter<int64_t>::ToV8(v8::Isolate* isolate,
                                              int64_t val) {
  return v8::Number::New(isolate, static_cast<double>(val));
}

bool Converter<int64_t>::FromV8(v8::Isolate* isolate,
                                v8::Local<v8::Value> val,
                                int64_t* out) {
  if (!val->IsNumber())
    return false;
  // Even though IntegerValue returns int64_t, JavaScript cannot represent
  // the full precision of int64_t, which means some rounding might occur.
  return FromMaybe(val->IntegerValue(isolate->GetCurrentContext()), out);
}

v8::Local<v8::Value> Converter<uint64_t>::ToV8(v8::Isolate* isolate,
                                               uint64_t val) {
  return v8::Number::New(isolate, static_cast<double>(val));
}

bool Converter<uint64_t>::FromV8(v8::Isolate* isolate,
                                 v8::Local<v8::Value> val,
                                 uint64_t* out) {
  if (!val->IsNumber())
    return false;
  return FromMaybe(val->IntegerValue(isolate->GetCurrentContext()), out);
}

v8::Local<v8::Value> Converter<float>::ToV8(v8::Isolate* isolate, float val) {
  return v8::Number::New(isolate, val);
}

bool Converter<float>::FromV8(v8::Isolate* isolate,
                              v8::Local<v8::Value> val,
                              float* out) {
  if (!val->IsNumber())
    return false;
  *out = static_cast<float>(val.As<v8::Number>()->Value());
  return true;
}

v8::Local<v8::Value> Converter<double>::ToV8(v8::Isolate* isolate, double val) {
  return v8::Number::New(isolate, val);
}

bool Converter<double>::FromV8(v8::Isolate* isolate,
                               v8::Local<v8::Value> val,
                               double* out) {
  if (!val->IsNumber())
    return false;
  *out = val.As<v8::Number>()->Value();
  return true;
}

v8::Local<v8::Value> Converter<const char*>::ToV8(v8::Isolate* isolate,
                                                  const char* val) {
  return v8::String::NewFromUtf8(isolate, val, v8::NewStringType::kNormal)
      .ToLocalChecked();
}

v8::Local<v8::Value> Converter<base::StringPiece>::ToV8(v8::Isolate* isolate,
                                                        base::StringPiece val) {
  return v8::String::NewFromUtf8(isolate, val.data(),
                                 v8::NewStringType::kNormal,
                                 static_cast<uint32_t>(val.length()))
      .ToLocalChecked();
}

v8::Local<v8::Value> Converter<std::string>::ToV8(v8::Isolate* isolate,
                                                  const std::string& val) {
  return Converter<base::StringPiece>::ToV8(isolate, val);
}

bool Converter<std::string>::FromV8(v8::Isolate* isolate,
                                    v8::Local<v8::Value> val,
                                    std::string* out) {
  if (!val->IsString())
    return false;
  v8::Local<v8::String> str = v8::Local<v8::String>::Cast(val);
  int length = str->Utf8Length(isolate);
  out->resize(length);
  str->WriteUtf8(isolate, &(*out)[0], length, nullptr,
                 v8::String::NO_NULL_TERMINATION);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::Function>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::Function> val) {
  return val;
}

bool Converter<v8::Local<v8::Function>>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                v8::Local<v8::Function>* out) {
  if (!val->IsFunction())
    return false;
  *out = v8::Local<v8::Function>::Cast(val);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::Object>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::Object> val) {
  return val;
}

bool Converter<v8::Local<v8::Object>>::FromV8(v8::Isolate* isolate,
                                              v8::Local<v8::Value> val,
                                              v8::Local<v8::Object>* out) {
  if (!val->IsObject())
    return false;
  *out = v8::Local<v8::Object>::Cast(val);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::String>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::String> val) {
  return val;
}

bool Converter<v8::Local<v8::String>>::FromV8(v8::Isolate* isolate,
                                              v8::Local<v8::Value> val,
                                              v8::Local<v8::String>* out) {
  if (!val->IsString())
    return false;
  *out = v8::Local<v8::String>::Cast(val);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::External>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::External> val) {
  return val;
}

bool Converter<v8::Local<v8::External>>::FromV8(v8::Isolate* isolate,
                                                v8::Local<v8::Value> val,
                                                v8::Local<v8::External>* out) {
  if (!val->IsExternal())
    return false;
  *out = v8::Local<v8::External>::Cast(val);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::Array>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::Array> val) {
  return val;
}

bool Converter<v8::Local<v8::Array>>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             v8::Local<v8::Array>* out) {
  if (!val->IsArray())
    return false;
  *out = v8::Local<v8::Array>::Cast(val);
  return true;
}

v8::Local<v8::Value> Converter<v8::Local<v8::Value>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val) {
  return val;
}

v8::Local<v8::Promise> Converter<v8::Local<v8::Promise>>::ToV8(
    v8::Isolate* isolate,
    v8::Local<v8::Promise> val) {
  return val;
}

bool Converter<v8::Local<v8::Value>>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             v8::Local<v8::Value>* out) {
  *out = val;
  return true;
}

v8::Local<v8::String> StringToSymbol(v8::Isolate* isolate,
                                     base::StringPiece val) {
  return v8::String::NewFromUtf8(isolate, val.data(),
                                 v8::NewStringType::kInternalized,
                                 static_cast<uint32_t>(val.length()))
      .ToLocalChecked();
}

}  // namespace mate
