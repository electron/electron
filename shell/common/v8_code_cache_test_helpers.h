// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_V8_CODE_CACHE_TEST_HELPERS_H_
#define ELECTRON_SHELL_COMMON_V8_CODE_CACHE_TEST_HELPERS_H_

#include <cstdint>
#include <string>

namespace electron::testing {

[[nodiscard]] uint32_t ComputeV8VersionHash(const std::string& embedder);

}  // namespace electron::testing

#endif  // ELECTRON_SHELL_COMMON_V8_CODE_CACHE_TEST_HELPERS_H_
