// Copyright (c) 2026 Anthropic GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "electron/shell/common/serialized_value_mojom_traits.h"

#include <utility>

namespace mojo {

// static
bool StructTraits<electron::mojom::SerializedValueDataView,
                  electron::SerializedValue>::
    Read(electron::mojom::SerializedValueDataView data,
         electron::SerializedValue* out) {
  mojo_base::BigBuffer buffer;
  if (!data.ReadBuffer(&buffer) || data.size() > buffer.size())
    return false;
  *out = electron::SerializedValue(std::move(buffer), data.size());
  return true;
}

}  // namespace mojo
