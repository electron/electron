// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/serialized_value.h"

#include <utility>

#include "base/check_op.h"

namespace electron {

SerializedValue::SerializedValue() = default;

SerializedValue::SerializedValue(mojo_base::BigBuffer buffer, size_t size)
    : buffer_(std::move(buffer)), size_(size) {
  CHECK_LE(size_, buffer_.size());
}

SerializedValue::SerializedValue(SerializedValue&& other)
    : buffer_(std::move(other.buffer_)), size_(std::exchange(other.size_, 0)) {}

SerializedValue& SerializedValue::operator=(SerializedValue&& other) {
  buffer_ = std::move(other.buffer_);
  size_ = std::exchange(other.size_, 0);
  return *this;
}

SerializedValue::~SerializedValue() = default;

}  // namespace electron
