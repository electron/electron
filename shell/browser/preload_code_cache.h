// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_

#include <string>
#include <vector>

#include "base/containers/span.h"

namespace electron::preload_code_cache {

// Returns the V8 code cache blob for the preload with the given |id|, or
// empty if no cache exists. Checks an in-memory tier first, then disk
// (userData/Code Cache/electron-preload/). Disk I/O is synchronous (call
// behind a ScopedAllowBlocking) and happens at most once per id per session
// — once read from (or absent on) disk, the result is cached in memory.
//
// Stale or corrupt blobs are not detected here. V8's CachedData validation
// rejects blobs from a different V8 version, flag set, or source — the
// renderer falls back to a normal compile and ships a fresh blob, which
// overwrites the stale one.
std::vector<uint8_t> Get(const std::string& id);

// Stores a V8 code cache blob for the preload with the given |id|. The
// in-memory tier is written immediately; the disk write is posted to the
// thread pool (fire-and-forget — a lost write just means the next launch
// pays one cold compile).
void Set(const std::string& id, base::span<const uint8_t> blob);

}  // namespace electron::preload_code_cache

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
