// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "crypto/hash.h"

namespace base {
class FilePath;
}

namespace content {
class RenderFrameHost;
}

class GURL;

namespace electron::preload_code_cache {

// sha256 of a preload script's source bytes.
using SourceHash = std::array<uint8_t, crypto::hash::kSha256Size>;

// One preload served to a frame: its cache id and the sha256 of the exact
// bytes the browser served.
using ServedPreload = std::pair<std::string, SourceHash>;

// Cache id for a webPreferences.preload script (stable across launches).
std::string IdForWebPreferencesPreload(const base::FilePath& path);

// Site key for cache entries produced or consumed by |rfh|'s document
// (the SiteInstance's site URL). Cache entries are keyed by (id, site) so a
// blob produced by one site's renderer is never served to documents of
// another site, and different-site consumers of the same preload don't evict
// each other's entries. May be empty for a default SiteInstance; an empty
// site only ever matches another empty site.
GURL SiteForFrame(content::RenderFrameHost* rfh);

// Returns the V8 code cache blob recorded for (|id|, |site|) and bound to
// source bytes whose sha256 is |source_hash| — or empty. Checks an in-memory
// tier first, then disk (userData/Code Cache/electron-preload/). Disk I/O is
// synchronous (call behind a ScopedAllowBlocking) and happens at most once
// per (id, site) per session.
//
// Entries are bound to the source hash: V8's own CachedData source check
// hashes only the source *length*, so without this a same-length source
// change would execute stale bytecode. V8 still validates version/flags at
// consume time.
std::vector<uint8_t> Get(const std::string& id,
                         const GURL& site,
                         const SourceHash& source_hash);

// Records the preloads — id plus the sha256 of the exact bytes — that the
// browser served to |rfh| in a RendererStartupData push, replacing any
// previous record for that frame. SetFromRenderer() only accepts cache
// writes for tuples recorded here: the browser, not the renderer, is the
// authority for which source bytes a cache entry validates against. The
// record's lifetime is tied to the frame (cleared when the frame or its
// WebContents goes away).
void RecordServedPreloads(content::RenderFrameHost* rfh,
                          std::vector<ServedPreload> served);

// Stores a code cache blob reported by |rfh|'s renderer for a preload that
// was served to it. The write is dropped unless RecordServedPreloads()
// recorded the id for this exact frame and |claimed_source_hash| equals the
// browser-recorded hash of the served bytes. The claimed hash is only a
// cross-check (it detects a write racing a newer push after the preload
// changed on disk); what is persisted is the browser-recorded hash, keyed by
// the frame's site — the renderer never supplies the metadata that future
// loads validate against. The in-memory tier is written immediately; the
// disk write is fire-and-forget on the thread pool.
void SetFromRenderer(content::RenderFrameHost* rfh,
                     const std::string& id,
                     base::span<const uint8_t> claimed_source_hash,
                     base::span<const uint8_t> blob);

}  // namespace electron::preload_code_cache

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
