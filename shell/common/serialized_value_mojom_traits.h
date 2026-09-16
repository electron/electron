// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_MOJOM_TRAITS_H_
#define ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_MOJOM_TRAITS_H_

#include <stdint.h>

#include "electron/shell/common/api/api.mojom-shared.h"
#include "electron/shell/common/serialized_value.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/base/big_buffer_mojom_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct StructTraits<electron::mojom::SerializedValueDataView,
                    electron::SerializedValue> {
  static mojo_base::BigBuffer& buffer(electron::SerializedValue& value) {
    return value.buffer();
  }
  static uint64_t size(const electron::SerializedValue& value) {
    return value.size();
  }
  static bool Read(electron::mojom::SerializedValueDataView data,
                   electron::SerializedValue* out);
};

}  // namespace mojo

#endif  // ELECTRON_SHELL_COMMON_SERIALIZED_VALUE_MOJOM_TRAITS_H_
