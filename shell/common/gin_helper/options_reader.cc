// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/options_reader.h"

#include <utility>

#include "base/strings/strcat.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper {

OptionsReader::OptionsReader(v8::Isolate* isolate,
                             v8::Local<v8::Object> object,
                             ConversionError& error,
                             std::string path)
    : isolate_(isolate),
      object_(object),
      error_(error),
      path_(std::move(path)) {}

OptionsReader::OptionsReader(const OptionsReader&) = default;
OptionsReader::~OptionsReader() = default;

// static
std::optional<OptionsReader> OptionsReader::Of(v8::Isolate* isolate,
                                               v8::Local<v8::Value> value,
                                               std::string_view name,
                                               ConversionError& error,
                                               std::string path) {
  if (value.IsEmpty() || !value->IsObject() || value->IsFunction()) {
    error.Expected(name, TypeDescription<v8::Local<v8::Object>>::value);
    return std::nullopt;
  }
  return OptionsReader(isolate, value.As<v8::Object>(), error, std::move(path));
}

std::string OptionsReader::PathOf(std::string_view key) const {
  if (path_.empty())
    return std::string(key);
  return base::StrCat({path_, ".", key});
}

bool OptionsReader::GetValue(std::string_view key,
                             v8::Local<v8::Value>* out) const {
  if (error_->failed())
    return false;
  v8::Local<v8::Value> value;
  if (!object_
           ->Get(isolate_->GetCurrentContext(), gin::StringToV8(isolate_, key))
           .ToLocal(&value)) {
    error_->Fail(base::StrCat({"Exception reading ", PathOf(key)}));
    return false;
  }
  if (value->IsNullOrUndefined())
    return false;
  *out = value;
  return true;
}

bool OptionsReader::Has(std::string_view key) const {
  v8::Local<v8::Value> value;
  return GetValue(key, &value);
}

std::optional<OptionsReader> OptionsReader::GetReader(
    std::string_view key) const {
  v8::Local<v8::Value> value;
  if (!GetValue(key, &value))
    return std::nullopt;
  std::string path = PathOf(key);
  return Of(isolate_, value, path, *error_, path);
}

}  // namespace gin_helper
