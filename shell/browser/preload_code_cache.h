// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_

#include <string>
#include <vector>

#include "base/containers/span.h"

namespace base {
class FilePath;
}

namespace electron::preload_code_cache {

// Cache id for a webPreferences.preload script (stable across launches).
std::string IdForWebPreferencesPreload(const base::FilePath& path);

// Returns the V8 code cache blob recorded for |id| and these exact
// |contents|, or empty. Checks an in-memory tier first, then disk
// (userData/Code Cache/electron-preload/). Disk I/O is synchronous (call
// behind a ScopedAllowBlocking) and happens at most once per id per session.
//
// Entries are bound to sha256(contents): V8's own CachedData source check
// hashes only the source *length*, so without this a same-length source
// change would execute stale bytecode. V8 still validates version/flags at
// consume time.
std::vector<uint8_t> Get(const std::string& id,
                         base::span<const uint8_t> contents);

// Stores a blob compiled from source whose sha256 is |source_hash| (32
// bytes; other sizes are ignored). The in-memory tier is written
// immediately; the disk write is fire-and-forget on the thread pool.
void Set(const std::string& id,
         base::span<const uint8_t> source_hash,
         base::span<const uint8_t> blob);

}  // namespace electron::preload_code_cache

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
