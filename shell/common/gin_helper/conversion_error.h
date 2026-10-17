// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERSION_ERROR_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERSION_ERROR_H_

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "v8/include/v8-forward.h"
#include "v8/include/v8-local-handle.h"

namespace gin_helper {

// Why converting a JavaScript value failed and, when it was a property of an
// options object, which one ("margins.top"), so that an API can throw or
// reject with "margins.top must be a number" rather than gin's generic
// conversion error. One ConversionError is handed down through OptionsReader
// and any converter that accepts one, so a failure several objects deep is
// still reported against its full path and with the JavaScript error class the
// API wants.
class ConversionError {
 public:
  enum class Kind { kError, kTypeError, kRangeError };

  ConversionError();
  ~ConversionError();
  ConversionError(const ConversionError&) = delete;
  ConversionError& operator=(const ConversionError&) = delete;

  bool failed() const { return message_.has_value(); }
  // Empty unless failed().
  std::string_view message() const;
  Kind kind() const { return kind_; }

  // Records a failure. Only the first failure is kept, so callers can keep
  // converting and check once at the end.
  void Fail(std::string_view message, Kind kind = Kind::kError);
  // Records "<path> must be <description>" as a TypeError, e.g.
  // Expected("margins.top", "a number").
  void Expected(std::string_view path, std::string_view description);

  // The failure as a JavaScript exception of kind().
  v8::Local<v8::Value> ToException(v8::Isolate* isolate) const;
  // Throws ToException() on |isolate|.
  void Throw(v8::Isolate* isolate) const;

 private:
  std::optional<std::string> message_;
  Kind kind_ = Kind::kError;
};

// How a value of type T is described in "<path> must be ..." messages.
// Specialise for types whose gin::Converter has no other way to say what it
// wanted.
template <typename T, typename = void>
struct TypeDescription {
  static constexpr std::string_view value = "";
};

template <>
struct TypeDescription<bool> {
  static constexpr std::string_view value = "a boolean";
};

template <typename T>
struct TypeDescription<
    T,
    std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>>> {
  static constexpr std::string_view value = "a number";
};

template <>
struct TypeDescription<std::string> {
  static constexpr std::string_view value = "a string";
};

template <>
struct TypeDescription<std::u16string> {
  static constexpr std::string_view value = "a string";
};

template <>
struct TypeDescription<v8::Local<v8::Function>> {
  static constexpr std::string_view value = "a function";
};

template <>
struct TypeDescription<v8::Local<v8::Object>> {
  static constexpr std::string_view value = "an object";
};

}  // namespace gin_helper

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_CONVERSION_ERROR_H_
