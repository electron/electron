// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_DICTIONARY_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_DICTIONARY_H_

#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "gin/dictionary.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/accessor.h"
#include "shell/common/gin_helper/function_template.h"

namespace gin_helper {

// Adds a few more extends methods to gin::Dictionary.
//
// Note that as the destructor of gin::Dictionary is not virtual, and we want to
// convert between 2 types, we must not add any member.
class Dictionary : public gin::Dictionary {
 public:
  Dictionary() : gin::Dictionary(nullptr) {}
  Dictionary(v8::Isolate* isolate, v8::Local<v8::Object> object)
      : gin::Dictionary(isolate, object) {}

  // Allow implicitly converting from gin::Dictionary, as it is absolutely
  // safe in this case.
  Dictionary(const gin::Dictionary& dict)  // NOLINT(runtime/explicit)
      : gin::Dictionary(dict) {}

  static Dictionary CreateEmpty(v8::Isolate* isolate) {
    return gin::Dictionary::CreateEmpty(isolate);
  }

  // Differences from the Get method in gin::Dictionary:
  // 1. This is a const method;
  // 2. It checks whether the key exists before reading;
  // 3. It accepts arbitrary type of key.
  template <typename K, typename V>
  bool Get(const K& key, V* out) const {
    v8::Isolate* const iso = isolate();
    v8::Local<v8::Object> handle = GetHandle();
    v8::Local<v8::Context> context = iso->GetCurrentContext();
    v8::Local<v8::Value> v8_key = gin::ConvertToV8(iso, key);
    v8::Local<v8::Value> value;
    // Check for existence before getting, otherwise this method will always
    // returns true when T == v8::Local<v8::Value>.
    return handle->Has(context, v8_key).FromMaybe(false) &&
           handle->Get(context, v8_key).ToLocal(&value) &&
           gin::ConvertFromV8(iso, value, out);
  }

  // Differences from the Set method in gin::Dictionary:
  // 1. It accepts arbitrary type of key.
  template <typename K, typename V>
  bool Set(const K& key, const V& val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result =
        GetHandle()->Set(isolate()->GetCurrentContext(),
                         gin::ConvertToV8(isolate(), key), v8_value);
    return result.FromMaybe(false);
  }

  // Like normal Get but put result in an std::optional.
  template <typename T>
  bool GetOptional(const std::string_view key, std::optional<T>* out) const {
    T ret;
    if (Get(key, &ret)) {
      out->emplace(std::move(ret));
      return true;
    } else {
      return false;
    }
  }

  template <typename T>
  bool GetHidden(std::string_view key, T* out) const {
    v8::Isolate* const iso = isolate();
    v8::Local<v8::Object> handle = GetHandle();
    v8::Local<v8::Context> context = iso->GetCurrentContext();
    v8::Local<v8::Private> privateKey = MakeHiddenKey(key);
    v8::Local<v8::Value> value;
    return handle->HasPrivate(context, privateKey).FromMaybe(false) &&
           handle->GetPrivate(context, privateKey).ToLocal(&value) &&
           gin::ConvertFromV8(iso, value, out);
  }

  template <typename T>
  bool SetHidden(std::string_view key, T val) {
    v8::Isolate* const iso = isolate();
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(iso, val, &v8_value))
      return false;
    v8::Local<v8::Context> context = iso->GetCurrentContext();
    v8::Local<v8::Private> privateKey = MakeHiddenKey(key);
    v8::Maybe<bool> result =
        GetHandle()->SetPrivate(context, privateKey, v8_value);
    return result.FromMaybe(false);
  }

  template <typename T>
  bool SetMethod(std::string_view key, const T& callback) {
    auto context = isolate()->GetCurrentContext();
    auto templ = CallbackTraits<T>::CreateTemplate(isolate(), callback);
    return GetHandle()
        ->Set(context, MakeKey(key),
              templ->GetFunction(context).ToLocalChecked())
        .ToChecked();
  }

  template <typename K, typename V>
  bool SetGetter(const K& key,
                 const V& val,
                 v8::PropertyAttribute attribute = v8::None) {
    AccessorValue<V> acc_value;
    acc_value.Value = val;

    v8::Local<v8::Value> v8_value_accessor;
    if (!gin::TryConvertToV8(isolate(), acc_value, &v8_value_accessor))
      return false;

    auto context = isolate()->GetCurrentContext();

    return GetHandle()
        ->SetNativeDataProperty(
            context, MakeKey(key),
            [](v8::Local<v8::Name> property_name,
               const v8::PropertyCallbackInfo<v8::Value>& info) {
              AccessorValue<V> acc_value;
              if (!gin::ConvertFromV8(info.GetIsolate(), info.Data(),
                                      &acc_value))
                return;

              V val = acc_value.Value;
              v8::Local<v8::Value> v8_value;
              if (gin::TryConvertToV8(info.GetIsolate(), val, &v8_value))
                info.GetReturnValue().Set(v8_value);
            },
            nullptr, v8_value_accessor, attribute)
        .ToChecked();
  }

  template <typename T>
  bool SetReadOnly(std::string_view key, const T& val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate()->GetCurrentContext(), MakeKey(key), v8_value, v8::ReadOnly);
    return result.FromMaybe(false);
  }

  // Note: If we plan to add more Set methods, consider adding an option instead
  // of copying code.
  template <typename T>
  bool SetReadOnlyNonConfigurable(std::string_view key, T val) {
    v8::Local<v8::Value> v8_value;
    if (!gin::TryConvertToV8(isolate(), val, &v8_value))
      return false;
    v8::Maybe<bool> result = GetHandle()->DefineOwnProperty(
        isolate()->GetCurrentContext(), MakeKey(key), v8_value,
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
    return result.FromMaybe(false);
  }

  bool Has(std::string_view key) const {
    v8::Maybe<bool> result =
        GetHandle()->Has(isolate()->GetCurrentContext(), MakeKey(key));
    return result.FromMaybe(false);
  }

  bool Delete(std::string_view key) {
    v8::Maybe<bool> result =
        GetHandle()->Delete(isolate()->GetCurrentContext(), MakeKey(key));
    return result.FromMaybe(false);
  }

  bool IsEmpty() const { return isolate() == nullptr || GetHandle().IsEmpty(); }

  v8::Local<v8::Object> GetHandle() const {
    return gin::ConvertToV8(isolate(),
                            *static_cast<const gin::Dictionary*>(this))
        .As<v8::Object>();
  }

 private:
  // DO NOT ADD ANY DATA MEMBER.

  [[nodiscard]] v8::Local<v8::String> MakeKey(
      const std::string_view key) const {
    return gin::StringToV8(isolate(), key);
  }

  [[nodiscard]] v8::Local<v8::Private> MakeHiddenKey(
      const std::string_view key) const {
    return v8::Private::ForApi(isolate(), MakeKey(key));
  }
};

}  // namespace gin_helper

namespace gin {

template <>
struct Converter<gin_helper::Dictionary> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   gin_helper::Dictionary val) {
    return val.GetHandle();
  }
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gin_helper::Dictionary* out) {
    gin::Dictionary gdict(isolate);
    if (!ConvertFromV8(isolate, val, &gdict))
      return false;
    *out = gin_helper::Dictionary(gdict);
    return true;
  }
};

}  // namespace gin

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_DICTIONARY_H_
