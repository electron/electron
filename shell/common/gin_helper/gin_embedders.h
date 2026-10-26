// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_GIN_HELPER_GIN_EMBEDDERS_H_
#define ELECTRON_SHELL_COMMON_GIN_HELPER_GIN_EMBEDDERS_H_

#include <cstdint>

#include "gin/public/gin_embedders.h"

namespace electron {

namespace internal {

// Keep this switch exhaustive so adding a GinEmbedder upstream fails
// the build instead of silently colliding with kEmbedderElectron.
constexpr bool IsKnownGinEmbedder(gin::GinEmbedder embedder) {
  switch (embedder) {
    case gin::kEmbedderNativeGin:
    case gin::kEmbedderBlink:
    case gin::kEmbedderPDFium:
    case gin::kEmbedderFuchsia:
      return true;
  }
  return false;
}

}  // namespace internal

// Extend gin embedder range.
enum ElectronGinEmbedder : uint16_t {
  kEmbedderElectron = gin::kEmbedderFuchsia + 1,
};

static_assert(internal::IsKnownGinEmbedder(gin::kEmbedderFuchsia));

}  // namespace electron

#endif  // ELECTRON_SHELL_COMMON_GIN_HELPER_GIN_EMBEDDERS_H_
