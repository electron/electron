// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/v8/v8_value_converter.h"

#include <string>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"

namespace atom {

V8ValueConverter::V8ValueConverter()
    : date_allowed_(false),
      reg_exp_allowed_(false),
      function_allowed_(false),
      strip_null_from_objects_(false),
      avoid_identity_hash_for_testing_(false) {}

void V8ValueConverter::SetDateAllowed(bool val) {
  date_allowed_ = val;
}

void V8ValueConverter::SetRegExpAllowed(bool val) {
  reg_exp_allowed_ = val;
}

void V8ValueConverter::SetFunctionAllowed(bool val) {
  function_allowed_ = val;
}

void V8ValueConverter::SetStripNullFromObjects(bool val) {
  strip_null_from_objects_ = val;
}

v8::Handle<v8::Value> V8ValueConverter::ToV8Value(
    const base::Value* value, v8::Handle<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  return handle_scope.Close(ToV8ValueImpl(value));
}

Value* V8ValueConverter::FromV8Value(
    v8::Handle<v8::Value> val,
    v8::Handle<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::HandleScope handle_scope(v8::Isolate::GetCurrent());
  HashToHandleMap unique_map;
  return FromV8ValueImpl(val, &unique_map);
}

v8::Handle<v8::Value> V8ValueConverter::ToV8ValueImpl(
     const base::Value* value) const {
  CHECK(value);
  switch (value->GetType()) {
    case base::Value::TYPE_NULL:
      return v8::Null();

    case base::Value::TYPE_BOOLEAN: {
      bool val = false;
      CHECK(value->GetAsBoolean(&val));
      return v8::Boolean::New(val);
    }

    case base::Value::TYPE_INTEGER: {
      int val = 0;
      CHECK(value->GetAsInteger(&val));
      return v8::Integer::New(val);
    }

    case base::Value::TYPE_DOUBLE: {
      double val = 0.0;
      CHECK(value->GetAsDouble(&val));
      return v8::Number::New(val);
    }

    case base::Value::TYPE_STRING: {
      std::string val;
      CHECK(value->GetAsString(&val));
      return v8::String::New(val.c_str(), val.length());
    }

    case base::Value::TYPE_LIST:
      return ToV8Array(static_cast<const base::ListValue*>(value));

    case base::Value::TYPE_DICTIONARY:
      return ToV8Object(static_cast<const base::DictionaryValue*>(value));

    default:
      LOG(ERROR) << "Unexpected value type: " << value->GetType();
      return v8::Null();
  }
}

v8::Handle<v8::Value> V8ValueConverter::ToV8Array(
    const base::ListValue* val) const {
  v8::Handle<v8::Array> result(v8::Array::New(val->GetSize()));

  for (size_t i = 0; i < val->GetSize(); ++i) {
    const base::Value* child = NULL;
    CHECK(val->Get(i, &child));

    v8::Handle<v8::Value> child_v8 = ToV8ValueImpl(child);
    CHECK(!child_v8.IsEmpty());

    v8::TryCatch try_catch;
    result->Set(static_cast<uint32>(i), child_v8);
    if (try_catch.HasCaught())
      LOG(ERROR) << "Setter for index " << i << " threw an exception.";
  }

  return result;
}

v8::Handle<v8::Value> V8ValueConverter::ToV8Object(
    const base::DictionaryValue* val) const {
  v8::Handle<v8::Object> result(v8::Object::New());

  for (base::DictionaryValue::Iterator iter(*val);
       !iter.IsAtEnd(); iter.Advance()) {
    const std::string& key = iter.key();
    v8::Handle<v8::Value> child_v8 = ToV8ValueImpl(&iter.value());
    CHECK(!child_v8.IsEmpty());

    v8::TryCatch try_catch;
    result->Set(v8::String::New(key.c_str(), key.length()), child_v8);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Setter for property " << key.c_str() << " threw an "
                 << "exception.";
    }
  }

  return result;
}

Value* V8ValueConverter::FromV8ValueImpl(v8::Handle<v8::Value> val,
    HashToHandleMap* unique_map) const {
  CHECK(!val.IsEmpty());

  if (val->IsNull())
    return base::Value::CreateNullValue();

  if (val->IsBoolean())
    return new base::FundamentalValue(val->ToBoolean()->Value());

  if (val->IsInt32())
    return new base::FundamentalValue(val->ToInt32()->Value());

  if (val->IsNumber())
    return new base::FundamentalValue(val->ToNumber()->Value());

  if (val->IsString()) {
    v8::String::Utf8Value utf8(val->ToString());
    return new base::StringValue(std::string(*utf8, utf8.length()));
  }

  if (val->IsUndefined())
    // JSON.stringify ignores undefined.
    return NULL;

  if (val->IsDate()) {
    if (!date_allowed_)
      // JSON.stringify would convert this to a string, but an object is more
      // consistent within this class.
      return FromV8Object(val->ToObject(), unique_map);
    v8::Date* date = v8::Date::Cast(*val);
    return new base::FundamentalValue(date->NumberValue() / 1000.0);
  }

  if (val->IsRegExp()) {
    if (!reg_exp_allowed_)
      // JSON.stringify converts to an object.
      return FromV8Object(val->ToObject(), unique_map);
    return new base::StringValue(*v8::String::Utf8Value(val->ToString()));
  }

  // v8::Value doesn't have a ToArray() method for some reason.
  if (val->IsArray())
    return FromV8Array(val.As<v8::Array>(), unique_map);

  if (val->IsFunction()) {
    if (!function_allowed_)
      // JSON.stringify refuses to convert function(){}.
      return NULL;
    return FromV8Object(val->ToObject(), unique_map);
  }

  if (val->IsObject()) {
    return FromV8Object(val->ToObject(), unique_map);
  }

  LOG(ERROR) << "Unexpected v8 value type encountered.";
  return NULL;
}

Value* V8ValueConverter::FromV8Array(v8::Handle<v8::Array> val,
    HashToHandleMap* unique_map) const {
  if (!UpdateAndCheckUniqueness(unique_map, val))
    return base::Value::CreateNullValue();

  scoped_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != v8::Context::GetCurrent())
    scope.reset(new v8::Context::Scope(val->CreationContext()));

  base::ListValue* result = new base::ListValue();

  // Only fields with integer keys are carried over to the ListValue.
  for (uint32 i = 0; i < val->Length(); ++i) {
    v8::TryCatch try_catch;
    v8::Handle<v8::Value> child_v8 = val->Get(i);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Getter for index " << i << " threw an exception.";
      child_v8 = v8::Null();
    }

    if (!val->HasRealIndexedProperty(i))
      continue;

    base::Value* child = FromV8ValueImpl(child_v8, unique_map);
    if (child)
      result->Append(child);
    else
      // JSON.stringify puts null in places where values don't serialize, for
      // example undefined and functions. Emulate that behavior.
      result->Append(base::Value::CreateNullValue());
  }
  return result;
}

Value* V8ValueConverter::FromV8Object(
    v8::Handle<v8::Object> val,
    HashToHandleMap* unique_map) const {
  if (!UpdateAndCheckUniqueness(unique_map, val))
    return base::Value::CreateNullValue();
  scoped_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != v8::Context::GetCurrent())
    scope.reset(new v8::Context::Scope(val->CreationContext()));

  scoped_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  v8::Handle<v8::Array> property_names(val->GetOwnPropertyNames());

  for (uint32 i = 0; i < property_names->Length(); ++i) {
    v8::Handle<v8::Value> key(property_names->Get(i));

    // Extend this test to cover more types as necessary and if sensible.
    if (!key->IsString() &&
        !key->IsNumber()) {
      NOTREACHED() << "Key \"" << *v8::String::AsciiValue(key) << "\" "
                      "is neither a string nor a number";
      continue;
    }

    // Skip all callbacks: crbug.com/139933
    if (val->HasRealNamedCallbackProperty(key->ToString()))
      continue;

    v8::String::Utf8Value name_utf8(key->ToString());

    v8::TryCatch try_catch;
    v8::Handle<v8::Value> child_v8 = val->Get(key);

    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Getter for property " << *name_utf8
                 << " threw an exception.";
      child_v8 = v8::Null();
    }

    scoped_ptr<base::Value> child(FromV8ValueImpl(child_v8, unique_map));
    if (!child.get())
      // JSON.stringify skips properties whose values don't serialize, for
      // example undefined and functions. Emulate that behavior.
      continue;

    // Strip null if asked (and since undefined is turned into null, undefined
    // too). The use case for supporting this is JSON-schema support,
    // specifically for extensions, where "optional" JSON properties may be
    // represented as null, yet due to buggy legacy code elsewhere isn't
    // treated as such (potentially causing crashes). For example, the
    // "tabs.create" function takes an object as its first argument with an
    // optional "windowId" property.
    //
    // Given just
    //
    //   tabs.create({})
    //
    // this will work as expected on code that only checks for the existence of
    // a "windowId" property (such as that legacy code). However given
    //
    //   tabs.create({windowId: null})
    //
    // there *is* a "windowId" property, but since it should be an int, code
    // on the browser which doesn't additionally check for null will fail.
    // We can avoid all bugs related to this by stripping null.
    if (strip_null_from_objects_ && child->IsType(Value::TYPE_NULL))
      continue;

    result->SetWithoutPathExpansion(std::string(*name_utf8, name_utf8.length()),
                                    child.release());
  }

  return result.release();
}

bool V8ValueConverter::UpdateAndCheckUniqueness(
    HashToHandleMap* map,
    v8::Handle<v8::Object> handle) const {
  typedef HashToHandleMap::const_iterator Iterator;

  int hash = avoid_identity_hash_for_testing_ ? 0 : handle->GetIdentityHash();
  // We only compare using == with handles to objects with the same identity
  // hash. Different hash obviously means different objects, but two objects in
  // a couple of thousands could have the same identity hash.
  std::pair<Iterator, Iterator> range = map->equal_range(hash);
  for (Iterator it = range.first; it != range.second; ++it) {
    // Operator == for handles actually compares the underlying objects.
    if (it->second == handle)
      return false;
  }

  map->insert(std::pair<int, v8::Handle<v8::Object> >(hash, handle));
  return true;
}

}  // namespace atom
