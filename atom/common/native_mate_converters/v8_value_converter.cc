// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/v8_value_converter.h"

#include <map>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "native_mate/dictionary.h"
#include "vendor/node/src/node_buffer.h"

namespace atom {

namespace {

const int kMaxRecursionDepth = 100;

}  // namespace

// The state of a call to FromV8Value.
class V8ValueConverter::FromV8ValueState {
 public:
  // Level scope which updates the current depth of some FromV8ValueState.
  class Level {
   public:
    explicit Level(FromV8ValueState* state) : state_(state) {
      state_->max_recursion_depth_--;
    }
    ~Level() {
      state_->max_recursion_depth_++;
    }

   private:
    FromV8ValueState* state_;
  };

  FromV8ValueState() : max_recursion_depth_(kMaxRecursionDepth) {}

  // If |handle| is not in |unique_map_|, then add it to |unique_map_| and
  // return true.
  //
  // Otherwise do nothing and return false. Here "A is unique" means that no
  // other handle B in the map points to the same object as A. Note that A can
  // be unique even if there already is another handle with the same identity
  // hash (key) in the map, because two objects can have the same hash.
  bool UpdateAndCheckUniqueness(v8::Local<v8::Object> handle) {
    typedef HashToHandleMap::const_iterator Iterator;
    int hash = handle->GetIdentityHash();
    // We only compare using == with handles to objects with the same identity
    // hash. Different hash obviously means different objects, but two objects
    // in a couple of thousands could have the same identity hash.
    std::pair<Iterator, Iterator> range = unique_map_.equal_range(hash);
    for (Iterator it = range.first; it != range.second; ++it) {
      // Operator == for handles actually compares the underlying objects.
      if (it->second == handle)
        return false;
    }
    unique_map_.insert(std::make_pair(hash, handle));
    return true;
  }

  bool HasReachedMaxRecursionDepth() {
    return max_recursion_depth_ < 0;
  }

 private:
  typedef std::multimap<int, v8::Local<v8::Object> > HashToHandleMap;
  HashToHandleMap unique_map_;

  int max_recursion_depth_;
};

V8ValueConverter::V8ValueConverter()
    : reg_exp_allowed_(false),
      function_allowed_(false),
      strip_null_from_objects_(false) {}

void V8ValueConverter::SetRegExpAllowed(bool val) {
  reg_exp_allowed_ = val;
}

void V8ValueConverter::SetFunctionAllowed(bool val) {
  function_allowed_ = val;
}

void V8ValueConverter::SetStripNullFromObjects(bool val) {
  strip_null_from_objects_ = val;
}

v8::Local<v8::Value> V8ValueConverter::ToV8Value(
    const base::Value* value, v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::EscapableHandleScope handle_scope(context->GetIsolate());
  return handle_scope.Escape(ToV8ValueImpl(context->GetIsolate(), value));
}

base::Value* V8ValueConverter::FromV8Value(
    v8::Local<v8::Value> val,
    v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::HandleScope handle_scope(context->GetIsolate());
  FromV8ValueState state;
  return FromV8ValueImpl(&state, val, context->GetIsolate());
}

v8::Local<v8::Value> V8ValueConverter::ToV8ValueImpl(
     v8::Isolate* isolate, const base::Value* value) const {
  CHECK(value);
  switch (value->GetType()) {
    case base::Value::TYPE_NULL:
      return v8::Null(isolate);

    case base::Value::TYPE_BOOLEAN: {
      bool val = false;
      CHECK(value->GetAsBoolean(&val));
      return v8::Boolean::New(isolate, val);
    }

    case base::Value::TYPE_INTEGER: {
      int val = 0;
      CHECK(value->GetAsInteger(&val));
      return v8::Integer::New(isolate, val);
    }

    case base::Value::TYPE_DOUBLE: {
      double val = 0.0;
      CHECK(value->GetAsDouble(&val));
      return v8::Number::New(isolate, val);
    }

    case base::Value::TYPE_STRING: {
      std::string val;
      CHECK(value->GetAsString(&val));
      return v8::String::NewFromUtf8(
          isolate, val.c_str(), v8::String::kNormalString, val.length());
    }

    case base::Value::TYPE_LIST:
      return ToV8Array(isolate, static_cast<const base::ListValue*>(value));

    case base::Value::TYPE_DICTIONARY:
      return ToV8Object(isolate,
                        static_cast<const base::DictionaryValue*>(value));

    case base::Value::TYPE_BINARY:
      return ToArrayBuffer(isolate,
                           static_cast<const base::BinaryValue*>(value));

    default:
      LOG(ERROR) << "Unexpected value type: " << value->GetType();
      return v8::Null(isolate);
  }
}

v8::Local<v8::Value> V8ValueConverter::ToV8Array(
    v8::Isolate* isolate, const base::ListValue* val) const {
  v8::Local<v8::Array> result(v8::Array::New(isolate, val->GetSize()));

  for (size_t i = 0; i < val->GetSize(); ++i) {
    const base::Value* child = NULL;
    CHECK(val->Get(i, &child));

    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, child);
    CHECK(!child_v8.IsEmpty());

    v8::TryCatch try_catch;
    result->Set(static_cast<uint32_t>(i), child_v8);
    if (try_catch.HasCaught())
      LOG(ERROR) << "Setter for index " << i << " threw an exception.";
  }

  return result;
}

v8::Local<v8::Value> V8ValueConverter::ToV8Object(
    v8::Isolate* isolate, const base::DictionaryValue* val) const {
  mate::Dictionary result = mate::Dictionary::CreateEmpty(isolate);
  result.SetHidden("simple", true);

  for (base::DictionaryValue::Iterator iter(*val);
       !iter.IsAtEnd(); iter.Advance()) {
    const std::string& key = iter.key();
    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, &iter.value());
    CHECK(!child_v8.IsEmpty());

    v8::TryCatch try_catch;
    result.Set(key, child_v8);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Setter for property " << key.c_str() << " threw an "
                 << "exception.";
    }
  }

  return result.GetHandle();
}

v8::Local<v8::Value> V8ValueConverter::ToArrayBuffer(
    v8::Isolate* isolate, const base::BinaryValue* value) const {
  return node::Buffer::Copy(isolate,
                            value->GetBuffer(),
                            value->GetSize()).ToLocalChecked();
}

base::Value* V8ValueConverter::FromV8ValueImpl(
    FromV8ValueState* state,
    v8::Local<v8::Value> val,
    v8::Isolate* isolate) const {
  CHECK(!val.IsEmpty());

  FromV8ValueState::Level state_level(state);
  if (state->HasReachedMaxRecursionDepth())
    return NULL;

  if (val->IsNull())
    return base::Value::CreateNullValue().release();

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
    v8::Date* date = v8::Date::Cast(*val);
    v8::Local<v8::Value> toISOString =
        date->Get(v8::String::NewFromUtf8(isolate, "toISOString"));
    if (toISOString->IsFunction()) {
      v8::Local<v8::Value> result =
          toISOString.As<v8::Function>()->Call(val, 0, nullptr);
      if (!result.IsEmpty()) {
        v8::String::Utf8Value utf8(result->ToString());
        return new base::StringValue(std::string(*utf8, utf8.length()));
      }
    }
  }

  if (val->IsRegExp()) {
    if (!reg_exp_allowed_)
      // JSON.stringify converts to an object.
      return FromV8Object(val->ToObject(), state, isolate);
    return new base::StringValue(*v8::String::Utf8Value(val->ToString()));
  }

  // v8::Value doesn't have a ToArray() method for some reason.
  if (val->IsArray())
    return FromV8Array(val.As<v8::Array>(), state, isolate);

  if (val->IsFunction()) {
    if (!function_allowed_)
      // JSON.stringify refuses to convert function(){}.
      return NULL;
    return FromV8Object(val->ToObject(), state, isolate);
  }

  if (node::Buffer::HasInstance(val)) {
    return FromNodeBuffer(val, state, isolate);
  }

  if (val->IsObject()) {
    return FromV8Object(val->ToObject(), state, isolate);
  }

  LOG(ERROR) << "Unexpected v8 value type encountered.";
  return NULL;
}

base::Value* V8ValueConverter::FromV8Array(
    v8::Local<v8::Array> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  if (!state->UpdateAndCheckUniqueness(val))
    return base::Value::CreateNullValue().release();

  std::unique_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope.reset(new v8::Context::Scope(val->CreationContext()));

  base::ListValue* result = new base::ListValue();

  // Only fields with integer keys are carried over to the ListValue.
  for (uint32_t i = 0; i < val->Length(); ++i) {
    v8::TryCatch try_catch;
    v8::Local<v8::Value> child_v8 = val->Get(i);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Getter for index " << i << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    if (!val->HasRealIndexedProperty(i))
      continue;

    base::Value* child = FromV8ValueImpl(state, child_v8, isolate);
    if (child)
      result->Append(child);
    else
      // JSON.stringify puts null in places where values don't serialize, for
      // example undefined and functions. Emulate that behavior.
      result->Append(base::Value::CreateNullValue());
  }
  return result;
}

base::Value* V8ValueConverter::FromNodeBuffer(
    v8::Local<v8::Value> value,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  return base::BinaryValue::CreateWithCopiedBuffer(
      node::Buffer::Data(value), node::Buffer::Length(value));
}

base::Value* V8ValueConverter::FromV8Object(
    v8::Local<v8::Object> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  if (!state->UpdateAndCheckUniqueness(val))
    return base::Value::CreateNullValue().release();

  std::unique_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope.reset(new v8::Context::Scope(val->CreationContext()));

  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  v8::Local<v8::Array> property_names(val->GetOwnPropertyNames());

  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> key(property_names->Get(i));

    // Extend this test to cover more types as necessary and if sensible.
    if (!key->IsString() &&
        !key->IsNumber()) {
      NOTREACHED() << "Key \"" << *v8::String::Utf8Value(key) << "\" "
                      "is neither a string nor a number";
      continue;
    }

    // Skip all callbacks: crbug.com/139933
    if (val->HasRealNamedCallbackProperty(key->ToString()))
      continue;

    v8::String::Utf8Value name_utf8(key->ToString());

    v8::TryCatch try_catch;
    v8::Local<v8::Value> child_v8 = val->Get(key);

    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Getter for property " << *name_utf8
                 << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    std::unique_ptr<base::Value> child(FromV8ValueImpl(state, child_v8, isolate));
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
    if (strip_null_from_objects_ && child->IsType(base::Value::TYPE_NULL))
      continue;

    result->SetWithoutPathExpansion(std::string(*name_utf8, name_utf8.length()),
                                    child.release());
  }

  return result.release();
}

}  // namespace atom
