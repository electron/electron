// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_H_
#define ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/span.h"
#include "mojo/public/cpp/base/big_buffer.h"

namespace electron {

// A V8-serialized value held in the buffer it crosses the process boundary in:
// inline bytes for small values, a shared memory region above
// mojo_base::BigBuffer::kMaxInlineBytes. The region may be larger than the
// payload; only the first size() bytes are meaningful.
class SerializedValue {
 public:
  SerializedValue();
  SerializedValue(mojo_base::BigBuffer buffer, size_t size);
  SerializedValue(SerializedValue&&);
  SerializedValue& operator=(SerializedValue&&);
  SerializedValue(const SerializedValue&) = delete;
  SerializedValue& operator=(const SerializedValue&) = delete;
  ~SerializedValue();

  base::span<const uint8_t> bytes() const {
    return base::span(buffer_).first(size_);
  }
  size_t size() const { return size_; }
  bool is_shared_memory() const {
    return buffer_.storage_type() ==
           mojo_base::BigBuffer::StorageType::kSharedMemory;
  }

  mojo_base::BigBuffer& buffer() { return buffer_; }

 private:
  mojo_base::BigBuffer buffer_;
  size_t size_ = 0;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_H_
