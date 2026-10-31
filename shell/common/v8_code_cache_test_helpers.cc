// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/v8_code_cache_test_helpers.h"

#include "base/dcheck_is_on.h"

#if DCHECK_IS_ON()
#include "v8/include/v8-version.h"
#include "v8/src/base/hashing.h"  // nogncheck
#include "v8/src/base/vector.h"   // nogncheck

namespace electron::testing {

uint32_t ComputeV8VersionHash(const std::string& embedder) {
  // Mirror Version::Hash without changing the process-wide embedder string.
  v8::base::Hasher hasher;
  hasher.Add(V8_MAJOR_VERSION)
      .Add(V8_MINOR_VERSION)
      .Add(V8_BUILD_NUMBER)
      .Add(V8_PATCH_LEVEL);
  hasher.AddRange(v8::base::OneByteVector(embedder.c_str()));
  return static_cast<uint32_t>(hasher.hash());
}

}  // namespace electron::testing
#endif
