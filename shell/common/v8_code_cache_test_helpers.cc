// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_code_cache_test_helpers.h"

#include "base/dcheck_is_on.h"

#if DCHECK_IS_ON()
#include "v8/test/common/version-utils.h"  // nogncheck

namespace electron::testing {

uint32_t ComputeV8VersionHash(const std::string& embedder) {
  if (embedder == v8::internal::Version::GetEmbedder())
    return v8::internal::Version::Hash();
  v8::internal::ScopedVersionEmbedderString scoped_embedder(embedder.c_str());
  return v8::internal::Version::Hash();
}

}  // namespace electron::testing
#endif
