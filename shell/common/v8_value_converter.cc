// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_value_converter.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/values.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_bindings.h"
#include "shell/common/node_includes.h"

namespace electron {

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
    ~Level() { state_->max_recursion_depth_++; }

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
  bool AddToUniquenessCheck(v8::Local<v8::Object> handle) {
    int hash;
    auto iter = GetIteratorInMap(handle, &hash);
    if (iter != unique_map_.end())
      return false;

    unique_map_.insert(std::make_pair(hash, handle));
    return true;
  }

  bool RemoveFromUniquenessCheck(v8::Local<v8::Object> handle) {
    int unused_hash;
    auto iter = GetIteratorInMap(handle, &unused_hash);
    if (iter == unique_map_.end())
      return false;
    unique_map_.erase(iter);
    return true;
  }

  bool HasReachedMaxRecursionDepth() { return max_recursion_depth_ < 0; }

 private:
  using HashToHandleMap = std::multimap<int, v8::Local<v8::Object>>;
  using Iterator = HashToHandleMap::const_iterator;

  Iterator GetIteratorInMap(v8::Local<v8::Object> handle, int* hash) {
    *hash = handle->GetIdentityHash();
    // We only compare using == with handles to objects with the same identity
    // hash. Different hash obviously means different objects, but two objects
    // in a couple of thousands could have the same identity hash.
    std::pair<Iterator, Iterator> range = unique_map_.equal_range(*hash);
    for (auto it = range.first; it != range.second; ++it) {
      // Operator == for handles actually compares the underlying objects.
      if (it->second == handle)
        return it;
    }
    // Not found.
    return unique_map_.end();
  }

  HashToHandleMap unique_map_;

  int max_recursion_depth_;
};

// A class to ensure that objects/arrays that are being converted by
// this V8ValueConverterImpl do not have cycles.
//
// An example of cycle: var v = {}; v = {key: v};
// Not an example of cycle: var v = {}; a = [v, v]; or w = {a: v, b: v};
class V8ValueConverter::ScopedUniquenessGuard {
 public:
  ScopedUniquenessGuard(V8ValueConverter::FromV8ValueState* state,
                        v8::Local<v8::Object> value)
      : state_(state),
        value_(value),
        is_valid_(state_->AddToUniquenessCheck(value_)) {}
  ~ScopedUniquenessGuard() {
    if (is_valid_) {
      bool removed = state_->RemoveFromUniquenessCheck(value_);
      DCHECK(removed);
    }
  }

  // disable copy
  ScopedUniquenessGuard(const ScopedUniquenessGuard&) = delete;
  ScopedUniquenessGuard& operator=(const ScopedUniquenessGuard&) = delete;

  bool is_valid() const { return is_valid_; }

 private:
  typedef std::multimap<int, v8::Local<v8::Object>> HashToHandleMap;
  V8ValueConverter::FromV8ValueState* state_;
  v8::Local<v8::Object> value_;
  bool is_valid_;
};

V8ValueConverter::V8ValueConverter() = default;

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
    const base::Value* value,
    v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::EscapableHandleScope handle_scope(context->GetIsolate());
  return handle_scope.Escape(ToV8ValueImpl(context->GetIsolate(), value));
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Value(
    v8::Local<v8::Value> val,
    v8::Local<v8::Context> context) const {
  v8::Context::Scope context_scope(context);
  v8::HandleScope handle_scope(context->GetIsolate());
  FromV8ValueState state;
  return FromV8ValueImpl(&state, val, context->GetIsolate());
}

v8::Local<v8::Value> V8ValueConverter::ToV8ValueImpl(
    v8::Isolate* isolate,
    const base::Value* value) const {
  switch (value->type()) {
    case base::Value::Type::NONE:
      return v8::Null(isolate);

    case base::Value::Type::BOOLEAN: {
      bool val = value->GetBool();
      return v8::Boolean::New(isolate, val);
    }

    case base::Value::Type::INTEGER: {
      int val = value->GetInt();
      return v8::Integer::New(isolate, val);
    }

    case base::Value::Type::DOUBLE: {
      double val = value->GetDouble();
      return v8::Number::New(isolate, val);
    }

    case base::Value::Type::STRING: {
      std::string val = value->GetString();
      return v8::String::NewFromUtf8(isolate, val.c_str(),
                                     v8::NewStringType::kNormal, val.length())
          .ToLocalChecked();
    }

    case base::Value::Type::LIST:
      return ToV8Array(isolate, static_cast<const base::ListValue*>(value));

    case base::Value::Type::DICTIONARY:
      return ToV8Object(isolate,
                        static_cast<const base::DictionaryValue*>(value));

    case base::Value::Type::BINARY:
      return ToArrayBuffer(isolate, static_cast<const base::Value*>(value));

    default:
      LOG(ERROR) << "Unexpected value type: " << value->type();
      return v8::Null(isolate);
  }
}

v8::Local<v8::Value> V8ValueConverter::ToV8Array(
    v8::Isolate* isolate,
    const base::ListValue* val) const {
  v8::Local<v8::Array> result(
      v8::Array::New(isolate, val->GetListDeprecated().size()));
  auto context = isolate->GetCurrentContext();

  for (size_t i = 0; i < val->GetListDeprecated().size(); ++i) {
    const base::Value& child = val->GetListDeprecated()[i];

    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, &child);

    v8::TryCatch try_catch(isolate);
    result->Set(context, static_cast<uint32_t>(i), child_v8).Check();
    if (try_catch.HasCaught())
      LOG(ERROR) << "Setter for index " << i << " threw an exception.";
  }

  return result;
}

v8::Local<v8::Value> V8ValueConverter::ToV8Object(
    v8::Isolate* isolate,
    const base::DictionaryValue* val) const {
  gin_helper::Dictionary result = gin::Dictionary::CreateEmpty(isolate);
  result.SetHidden("simple", true);

  for (base::DictionaryValue::Iterator iter(*val); !iter.IsAtEnd();
       iter.Advance()) {
    const std::string& key = iter.key();
    v8::Local<v8::Value> child_v8 = ToV8ValueImpl(isolate, &iter.value());

    v8::TryCatch try_catch(isolate);
    result.Set(key, child_v8);
    if (try_catch.HasCaught()) {
      LOG(ERROR) << "Setter for property " << key.c_str() << " threw an "
                 << "exception.";
    }
  }

  return result.GetHandle();
}

v8::Local<v8::Value> V8ValueConverter::ToArrayBuffer(
    v8::Isolate* isolate,
    const base::Value* value) const {
  const auto* data = reinterpret_cast<const char*>(value->GetBlob().data());
  size_t length = value->GetBlob().size();

  if (NodeBindings::IsInitialized()) {
    return node::Buffer::Copy(isolate, data, length).ToLocalChecked();
  }

  if (length > node::Buffer::kMaxLength) {
    return v8::Local<v8::Object>();
  }
  auto context = isolate->GetCurrentContext();
  auto array_buffer = v8::ArrayBuffer::New(isolate, length);
  std::shared_ptr<v8::BackingStore> backing_store =
      array_buffer->GetBackingStore();
  memcpy(backing_store->Data(), data, length);
  // From this point, if something goes wrong(can't find Buffer class for
  // example) we'll simply return a Uint8Array based on the created ArrayBuffer.
  // This can happen if no preload script was specified to the renderer.
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> buffer_value;

  // Get the Buffer class stored as a hidden value in the global object. We'll
  // use it return a browserified Buffer.
  if (!global.GetHidden("Buffer", &buffer_value) ||
      !buffer_value->IsFunction()) {
    return v8::Uint8Array::New(array_buffer, 0, length);
  }

  gin::Dictionary buffer_class(
      isolate,
      buffer_value->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
  v8::Local<v8::Value> from_value;
  if (!buffer_class.Get("from", &from_value) || !from_value->IsFunction()) {
    return v8::Uint8Array::New(array_buffer, 0, length);
  }

  v8::Local<v8::Value> args[] = {array_buffer};
  auto func = from_value.As<v8::Function>();
  auto result = func->Call(context, v8::Null(isolate), 1, args);
  if (!result.IsEmpty()) {
    return result.ToLocalChecked();
  }

  return v8::Uint8Array::New(array_buffer, 0, length);
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8ValueImpl(
    FromV8ValueState* state,
    v8::Local<v8::Value> val,
    v8::Isolate* isolate) const {
  FromV8ValueState::Level state_level(state);
  if (state->HasReachedMaxRecursionDepth())
    return nullptr;

  if (val->IsExternal())
    return std::make_unique<base::Value>();

  if (val->IsNull())
    return std::make_unique<base::Value>();

  auto context = isolate->GetCurrentContext();

  if (val->IsBoolean())
    return std::make_unique<base::Value>(val->ToBoolean(isolate)->Value());

  if (val->IsInt32())
    return std::make_unique<base::Value>(val.As<v8::Int32>()->Value());

  if (val->IsNumber()) {
    double val_as_double = val.As<v8::Number>()->Value();
    if (!std::isfinite(val_as_double))
      return nullptr;
    return std::make_unique<base::Value>(val_as_double);
  }

  if (val->IsString()) {
    v8::String::Utf8Value utf8(isolate, val);
    return std::make_unique<base::Value>(std::string(*utf8, utf8.length()));
  }

  if (val->IsUndefined())
    // JSON.stringify ignores undefined.
    return nullptr;

  if (val->IsDate()) {
    v8::Date* date = v8::Date::Cast(*val);
    v8::Local<v8::Value> toISOString =
        date->Get(context, v8::String::NewFromUtf8(isolate, "toISOString",
                                                   v8::NewStringType::kNormal)
                               .ToLocalChecked())
            .ToLocalChecked();
    if (toISOString->IsFunction()) {
      v8::MaybeLocal<v8::Value> result =
          toISOString.As<v8::Function>()->Call(context, val, 0, nullptr);
      if (!result.IsEmpty()) {
        v8::String::Utf8Value utf8(isolate, result.ToLocalChecked());
        return std::make_unique<base::Value>(std::string(*utf8, utf8.length()));
      }
    }
  }

  if (val->IsRegExp()) {
    if (!reg_exp_allowed_)
      // JSON.stringify converts to an object.
      return FromV8Object(val.As<v8::Object>(), state, isolate);
    return std::make_unique<base::Value>(*v8::String::Utf8Value(isolate, val));
  }

  // v8::Value doesn't have a ToArray() method for some reason.
  if (val->IsArray())
    return FromV8Array(val.As<v8::Array>(), state, isolate);

  if (val->IsFunction()) {
    if (!function_allowed_)
      // JSON.stringify refuses to convert function(){}.
      return nullptr;
    return FromV8Object(val.As<v8::Object>(), state, isolate);
  }

  if (node::Buffer::HasInstance(val)) {
    return FromNodeBuffer(val, state, isolate);
  }

  if (val->IsObject()) {
    return FromV8Object(val.As<v8::Object>(), state, isolate);
  }

  LOG(ERROR) << "Unexpected v8 value type encountered.";
  return nullptr;
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Array(
    v8::Local<v8::Array> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  ScopedUniquenessGuard uniqueness_guard(state, val);
  if (!uniqueness_guard.is_valid())
    return std::make_unique<base::Value>();

  std::unique_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope = std::make_unique<v8::Context::Scope>(val->CreationContext());

  auto result = std::make_unique<base::ListValue>();

  // Only fields with integer keys are carried over to the ListValue.
  for (uint32_t i = 0; i < val->Length(); ++i) {
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Value> child_v8;
    v8::MaybeLocal<v8::Value> maybe_child =
        val->Get(isolate->GetCurrentContext(), i);
    if (try_catch.HasCaught() || !maybe_child.ToLocal(&child_v8)) {
      LOG(ERROR) << "Getter for index " << i << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    if (!val->HasRealIndexedProperty(isolate->GetCurrentContext(), i)
             .FromMaybe(false)) {
      result->Append(std::make_unique<base::Value>());
      continue;
    }

    std::unique_ptr<base::Value> child =
        FromV8ValueImpl(state, child_v8, isolate);
    if (child)
      result->Append(std::move(child));
    else
      // JSON.stringify puts null in places where values don't serialize, for
      // example undefined and functions. Emulate that behavior.
      result->Append(std::make_unique<base::Value>());
  }
  return std::move(result);
}

std::unique_ptr<base::Value> V8ValueConverter::FromNodeBuffer(
    v8::Local<v8::Value> value,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  std::vector<char> buffer(
      node::Buffer::Data(value),
      node::Buffer::Data(value) + node::Buffer::Length(value));
  return std::make_unique<base::Value>(std::move(buffer));
}

std::unique_ptr<base::Value> V8ValueConverter::FromV8Object(
    v8::Local<v8::Object> val,
    FromV8ValueState* state,
    v8::Isolate* isolate) const {
  ScopedUniquenessGuard uniqueness_guard(state, val);
  if (!uniqueness_guard.is_valid())
    return std::make_unique<base::Value>();

  std::unique_ptr<v8::Context::Scope> scope;
  // If val was created in a different context than our current one, change to
  // that context, but change back after val is converted.
  if (!val->CreationContext().IsEmpty() &&
      val->CreationContext() != isolate->GetCurrentContext())
    scope = std::make_unique<v8::Context::Scope>(val->CreationContext());

  auto result = std::make_unique<base::DictionaryValue>();
  v8::Local<v8::Array> property_names;
  if (!val->GetOwnPropertyNames(isolate->GetCurrentContext())
           .ToLocal(&property_names)) {
    return std::move(result);
  }

  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> key =
        property_names->Get(isolate->GetCurrentContext(), i).ToLocalChecked();

    // Extend this test to cover more types as necessary and if sensible.
    if (!key->IsString() && !key->IsNumber()) {
      NOTREACHED() << "Key \"" << *v8::String::Utf8Value(isolate, key)
                   << "\" "
                      "is neither a string nor a number";
      continue;
    }

    v8::String::Utf8Value name_utf8(isolate, key);

    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Value> child_v8;
    v8::MaybeLocal<v8::Value> maybe_child =
        val->Get(isolate->GetCurrentContext(), key);
    if (try_catch.HasCaught() || !maybe_child.ToLocal(&child_v8)) {
      LOG(ERROR) << "Getter for property " << *name_utf8
                 << " threw an exception.";
      child_v8 = v8::Null(isolate);
    }

    std::unique_ptr<base::Value> child =
        FromV8ValueImpl(state, child_v8, isolate);
    if (!child)
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
    if (strip_null_from_objects_ && child->is_none())
      continue;

    result->SetWithoutPathExpansion(std::string(*name_utf8, name_utf8.length()),
                                    std::move(child));
  }

  return std::move(result);
}

}  // namespace electron
