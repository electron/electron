// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_V8_VALUE_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_V8_VALUE_CONVERTER_H_

#include <map>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "v8/include/v8.h"

namespace base {
class BinaryValue;
class DictionaryValue;
class ListValue;
class Value;
}

namespace atom {

class V8ValueConverter {
 public:
  V8ValueConverter();

  void SetDateAllowed(bool val);
  void SetRegExpAllowed(bool val);
  void SetFunctionAllowed(bool val);
  void SetStripNullFromObjects(bool val);
  v8::Local<v8::Value> ToV8Value(const base::Value* value,
                                 v8::Local<v8::Context> context) const;
  base::Value* FromV8Value(v8::Local<v8::Value> value,
                           v8::Local<v8::Context> context) const;

 private:
  typedef std::multimap<int, v8::Local<v8::Object> > HashToHandleMap;

  v8::Local<v8::Value> ToV8ValueImpl(v8::Isolate* isolate,
                                     const base::Value* value) const;
  v8::Local<v8::Value> ToV8Array(v8::Isolate* isolate,
                                 const base::ListValue* list) const;
  v8::Local<v8::Value> ToV8Object(
      v8::Isolate* isolate,
      const base::DictionaryValue* dictionary) const;

  base::Value* FromV8ValueImpl(v8::Local<v8::Value> value,
                               HashToHandleMap* unique_map) const;
  base::Value* FromV8Array(v8::Local<v8::Array> array,
                           HashToHandleMap* unique_map) const;

  base::Value* FromV8Object(v8::Local<v8::Object> object,
                            HashToHandleMap* unique_map) const;

  // If |handle| is not in |map|, then add it to |map| and return true.
  // Otherwise do nothing and return false. Here "A is unique" means that no
  // other handle B in the map points to the same object as A. Note that A can
  // be unique even if there already is another handle with the same identity
  // hash (key) in the map, because two objects can have the same hash.
  bool UpdateAndCheckUniqueness(HashToHandleMap* map,
                                v8::Local<v8::Object> handle) const;

  // If true, we will convert Date JavaScript objects to doubles.
  bool date_allowed_;

  // If true, we will convert RegExp JavaScript objects to string.
  bool reg_exp_allowed_;

  // If true, we will convert Function JavaScript objects to dictionaries.
  bool function_allowed_;

  // If true, undefined and null values are ignored when converting v8 objects
  // into Values.
  bool strip_null_from_objects_;

  bool avoid_identity_hash_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(V8ValueConverter);
};

}  // namespace atom

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_V8_VALUE_CONVERTER_H_
