// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_OPTIONS_READER_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_OPTIONS_READER_H_

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "gin/converter.h"
#include "shell/common/gin_helper/conversion_error.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"

namespace gin_helper {

// Converts |value| to T, recording why not on |error| against |path|. A
// gin::Converter<T> that can say more than "wrong type" may define
//   static bool FromV8(v8::Isolate*, v8::Local<v8::Value>, T* out,
//                      ConversionError& error, std::string_view path);
// and is then given the error and path to report (and to hand further down);
// any other converter's failure is reported as
// "<path> must be <TypeDescription<T>>".
template <typename T>
bool ConvertFromV8(v8::Isolate* isolate,
                   v8::Local<v8::Value> value,
                   T* out,
                   ConversionError& error,
                   std::string_view path) {
  if constexpr (requires {
                  gin::Converter<T>::FromV8(isolate, value, out, error, path);
                }) {
    return gin::Converter<T>::FromV8(isolate, value, out, error, path);
  } else if constexpr (std::is_same_v<T, bool>) {
    // gin's bool converter takes any value's truthiness; an option declared
    // boolean must be one.
    if (!value->IsBoolean()) {
      error.Expected(path, TypeDescription<bool>::value);
      return false;
    }
    *out = value.As<v8::Boolean>()->Value();
    return true;
  } else {
    if (gin::Converter<T>::FromV8(isolate, value, out))
      return true;
    error.Expected(path, TypeDescription<T>::value);
    return false;
  }
}

// Reads the properties of a JavaScript options object one at a time,
// converting each with ConvertFromV8 and recording the first failure on a
// shared ConversionError under the property's path from the outermost object
// ("pageSize.width"). A property whose value is undefined or null counts as
// absent, as it would for `options.key ?? fallback`. Once error() has failed
// (including because a getter threw; the exception is left pending for the
// caller's v8::TryCatch) nothing further is read, so later getters do not run,
// as if each read had thrown at the first failure.
class OptionsReader {
 public:
  // |path| is how this object is referred to in messages about its
  // properties; empty for the outermost object.
  OptionsReader(v8::Isolate* isolate,
                v8::Local<v8::Object> object,
                ConversionError& error,
                std::string path = std::string());
  OptionsReader(const OptionsReader&);
  ~OptionsReader();

  // An OptionsReader over |value| if it is an object (and not a function);
  // otherwise records "<name> must be an object" and returns nullopt.
  static std::optional<OptionsReader> Of(v8::Isolate* isolate,
                                         v8::Local<v8::Value> value,
                                         std::string_view name,
                                         ConversionError& error,
                                         std::string path = std::string());

  v8::Isolate* isolate() const { return isolate_; }
  v8::Local<v8::Object> object() const { return object_; }
  ConversionError& error() const { return *error_; }
  bool failed() const { return error_->failed(); }

  // "<path>.<key>", or just "<key>" at the top.
  std::string PathOf(std::string_view key) const;

  // The property's value if present (not undefined or null). False, without
  // reading, once error() has failed.
  bool GetValue(std::string_view key, v8::Local<v8::Value>* out) const;
  bool Has(std::string_view key) const;

  // Converts the property into *out if present. Returns false only when it is
  // present and does not convert, which is also recorded on error(); *out is
  // untouched unless conversion succeeds.
  template <typename T>
  bool Get(std::string_view key, T* out) const {
    v8::Local<v8::Value> value;
    if (!GetValue(key, &value))
      return true;
    return ConvertFromV8(isolate_, value, out, *error_, PathOf(key));
  }

  // A reader over the property if present; nullopt if absent, and nullopt
  // with "<path.key> must be an object" recorded if it is not an object.
  std::optional<OptionsReader> GetReader(std::string_view key) const;

 private:
  raw_ptr<v8::Isolate> isolate_;
  v8::Local<v8::Object> object_;
  raw_ref<ConversionError> error_;
  std::string path_;
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_OPTIONS_READER_H_
