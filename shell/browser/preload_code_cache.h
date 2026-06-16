// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
#define ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "content/public/browser/global_routing_id.h"
#include "shell/common/api/api.mojom.h"

namespace base {
class FilePath;
}

namespace content {
class RenderFrameHost;
}

class GURL;

namespace electron::preload_code_cache {

// Cache id for a webPreferences.preload script (stable across launches).
std::string IdForWebPreferencesPreload(const base::FilePath& path);

// Site key for cache entries produced or consumed by |rfh|'s document
// (the SiteInstance's site URL). Cache entries are bound to the site of the
// renderer that produced the blob and are only served back to documents of
// the same site, so a compromised renderer can never plant bytecode that a
// different site's (or a more trusted window's) preload realm would consume.
// May be empty for a default SiteInstance; an empty site only ever matches
// another empty site.
GURL SiteForFrame(content::RenderFrameHost* rfh);

// Returns the V8 code cache blob recorded for |id|, produced by a renderer
// of |site|, and bound to these exact |contents| — or empty. Checks an
// in-memory tier first, then disk (userData/Code Cache/electron-preload/).
// Disk I/O is synchronous (call behind a ScopedAllowBlocking) and happens at
// most once per id per session.
//
// Entries are bound to sha256(contents): V8's own CachedData source check
// hashes only the source *length*, so without this a same-length source
// change would execute stale bytecode. V8 still validates version/flags at
// consume time.
std::vector<uint8_t> Get(const std::string& id,
                         const GURL& site,
                         base::span<const uint8_t> contents);

// Records the preload scripts — and the sha256 of the exact bytes — that the
// browser served to |frame_token| in a RendererStartupData push, replacing
// any previous record for that frame. SetFromRenderer() only accepts cache
// writes for tuples recorded here: the browser, not the renderer, is the
// authority for which source bytes a cache entry validates against.
void RecordServedPreloads(
    const content::GlobalRenderFrameHostToken& frame_token,
    const std::vector<mojom::PreloadScriptDataPtr>& scripts);

// Drops the served-preload bookkeeping for a frame that is going away.
void OnRenderFrameDeleted(
    const content::GlobalRenderFrameHostToken& frame_token);

// Stores a code cache blob reported by |rfh|'s renderer for a preload that
// was served to it. The write is dropped unless RecordServedPreloads()
// recorded |id| for this exact frame and |claimed_source_hash| equals the
// browser-recorded hash of the served bytes. The claimed hash is only a
// cross-check (it detects a write racing a newer push after the preload
// changed on disk); what is persisted is the browser-recorded hash plus the
// frame's site — the renderer never supplies the metadata that future loads
// validate against. The in-memory tier is written immediately; the disk
// write is fire-and-forget on the thread pool.
void SetFromRenderer(content::RenderFrameHost* rfh,
                     const std::string& id,
                     base::span<const uint8_t> claimed_source_hash,
                     base::span<const uint8_t> blob);

}  // namespace electron::preload_code_cache

#endif  // ELECTRON_SHELL_BROWSER_PRELOAD_CODE_CACHE_H_
