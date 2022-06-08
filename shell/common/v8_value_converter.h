// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_V8_VALUE_CONVERTER_H_
#define ELECTRON_SHELL_COMMON_V8_VALUE_CONVERTER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "v8/include/v8.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}  // namespace base

namespace electron {

class V8ValueConverter {
 public:
  V8ValueConverter();

  // disable copy
  V8ValueConverter(const V8ValueConverter&) = delete;
  V8ValueConverter& operator=(const V8ValueConverter&) = delete;

  void SetRegExpAllowed(bool val);
  void SetFunctionAllowed(bool val);
  void SetStripNullFromObjects(bool val);
  v8::Local<v8::Value> ToV8Value(const base::Value* value,
                                 v8::Local<v8::Context> context) const;
  std::unique_ptr<base::Value> FromV8Value(
      v8::Local<v8::Value> value,
      v8::Local<v8::Context> context) const;

 private:
  class FromV8ValueState;
  class ScopedUniquenessGuard;

  v8::Local<v8::Value> ToV8ValueImpl(v8::Isolate* isolate,
                                     const base::Value* value) const;
  v8::Local<v8::Value> ToV8Array(v8::Isolate* isolate,
                                 const base::ListValue* list) const;
  v8::Local<v8::Value> ToV8Object(
      v8::Isolate* isolate,
      const base::DictionaryValue* dictionary) const;
  v8::Local<v8::Value> ToArrayBuffer(v8::Isolate* isolate,
                                     const base::Value* value) const;

  std::unique_ptr<base::Value> FromV8ValueImpl(FromV8ValueState* state,
                                               v8::Local<v8::Value> value,
                                               v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromV8Array(v8::Local<v8::Array> array,
                                           FromV8ValueState* state,
                                           v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromNodeBuffer(v8::Local<v8::Value> value,
                                              FromV8ValueState* state,
                                              v8::Isolate* isolate) const;
  std::unique_ptr<base::Value> FromV8Object(v8::Local<v8::Object> object,
                                            FromV8ValueState* state,
                                            v8::Isolate* isolate) const;

  // If true, we will convert RegExp JavaScript objects to string.
  bool reg_exp_allowed_ = false;

  // If true, we will convert Function JavaScript objects to dictionaries.
  bool function_allowed_ = false;

  // If true, undefined and null values are ignored when converting v8 objects
  // into Values.
  bool strip_null_from_objects_ = false;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_V8_VALUE_CONVERTER_H_
