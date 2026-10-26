// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/conversion_error.h"

#include "base/strings/strcat.h"
#include "gin/converter.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-isolate.h"

namespace gin_helper {

ConversionError::ConversionError() = default;
ConversionError::~ConversionError() = default;

std::string_view ConversionError::message() const {
  return message_ ? std::string_view(*message_) : std::string_view();
}

void ConversionError::Fail(std::string_view message, Kind kind) {
  if (message_)
    return;
  message_ = std::string(message);
  kind_ = kind;
}

void ConversionError::Expected(std::string_view path,
                               std::string_view description) {
  if (description.empty())
    Fail(base::StrCat({"Invalid value for ", path}), Kind::kTypeError);
  else
    Fail(base::StrCat({path, " must be ", description}), Kind::kTypeError);
}

v8::Local<v8::Value> ConversionError::ToException(v8::Isolate* isolate) const {
  v8::Local<v8::String> message = gin::StringToV8(isolate, this->message());
  switch (kind_) {
    case Kind::kTypeError:
      return v8::Exception::TypeError(message);
    case Kind::kRangeError:
      return v8::Exception::RangeError(message);
    case Kind::kError:
      return v8::Exception::Error(message);
  }
}

void ConversionError::Throw(v8::Isolate* isolate) const {
  isolate->ThrowException(ToException(isolate));
}

}  // namespace gin_helper
