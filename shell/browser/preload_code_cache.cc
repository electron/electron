// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/preload_code_cache.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

namespace electron::preload_code_cache {

namespace {

// Disk format: kMagic + sha256(source) + blob. Pre-hash-format files fail
// the magic check and read as a miss.
constexpr std::array<uint8_t, 4> kMagic{'E', 'P', 'C', '1'};

struct Entry {
  SourceHash source_hash;
  std::vector<uint8_t> blob;
};

// In-memory tier, keyed by (id, site). The optional<> memoizes a disk miss so
// it isn't re-tried per navigation. UI-thread only: both Get() (from the
// navigation push paths) and SetFromRenderer() (from the
// ElectronWebContentsUtility associated receiver on the RenderFrameHost) are
// dispatched on the UI thread, so the map needs no lock and concurrent
// SetPreloadCodeCache messages can't race it.
using Cache = base::flat_map<std::string, std::optional<Entry>>;
Cache& GetMemoryCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  static base::NoDestructor<Cache> cache;
  return *cache;
}

// sha256(served contents) per preload id, recorded per frame when the
// browser pushes RendererStartupData. This is the trust anchor for
// SetFromRenderer(): a renderer can only persist a cache entry for a preload
// the browser served to that exact frame, and only under the hash the
// browser computed from the bytes it served. UI-thread only.
using ServedHashes = base::flat_map<std::string, SourceHash>;
using ServedPreloads =
    base::flat_map<content::GlobalRenderFrameHostToken, ServedHashes>;
ServedPreloads& GetServedPreloads() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  static base::NoDestructor<ServedPreloads> served;
  return *served;
}

// Erases the served-preload records of a WebContents' frames when they (or
// the WebContents itself) go away, so GetServedPreloads() can't accumulate
// entries for dead frames. Owned by the WebContents it observes, so cleanup
// doesn't depend on any Electron-side wrapper existing for that WebContents
// (window.open contents are recorded before api::WebContents adopts them).
class ServedPreloadTracker final
    : private content::WebContentsObserver,
      public content::WebContentsUserData<ServedPreloadTracker> {
 public:
  ~ServedPreloadTracker() override { Purge(); }

  static void Track(content::RenderFrameHost* rfh) {
    auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
    if (!web_contents)
      return;
    CreateForWebContents(web_contents);
    FromWebContents(web_contents)->tokens_.insert(rfh->GetGlobalFrameToken());
  }

 private:
  friend class content::WebContentsUserData<ServedPreloadTracker>;
  explicit ServedPreloadTracker(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        content::WebContentsUserData<ServedPreloadTracker>(*web_contents) {}

  // content::WebContentsObserver:
  void RenderFrameDeleted(content::RenderFrameHost* rfh) override {
    const content::GlobalRenderFrameHostToken token =
        rfh->GetGlobalFrameToken();
    if (tokens_.erase(token))
      GetServedPreloads().erase(token);
  }
  void WebContentsDestroyed() override { Purge(); }

  void Purge() {
    for (const auto& token : tokens_)
      GetServedPreloads().erase(token);
    tokens_.clear();
  }

  base::flat_set<content::GlobalRenderFrameHostToken> tokens_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(ServedPreloadTracker);

// One slot per (id, producing site): different-site consumers of the same
// preload keep separate entries instead of evicting each other's.
std::string CacheKey(const std::string& id, const GURL& site) {
  return site.possibly_invalid_spec() + '\n' + id;
}

base::FilePath PathForEntry(const std::string& id, const GURL& site) {
  base::FilePath dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &dir))
    return {};
  // sha256(id)-sha256(site) keeps the filename filesystem-safe and bounded
  // while giving each (preload, site) pair its own file.
  std::string digest =
      base::HexEncode(crypto::hash::Sha256(id)) + "-" +
      base::HexEncode(crypto::hash::Sha256(site.possibly_invalid_spec()));
  return dir.AppendASCII("Code Cache")
      .AppendASCII("electron-preload")
      .AppendASCII(digest + ".cache");
}

std::optional<Entry> ParseDiskEntry(const std::string& file_contents) {
  base::span<const uint8_t> bytes = base::as_byte_span(file_contents);
  if (bytes.size() <= kMagic.size() + crypto::hash::kSha256Size)
    return std::nullopt;
  if (bytes.first<kMagic.size()>() != base::span(kMagic))
    return std::nullopt;
  Entry entry;
  base::span<const uint8_t> rest = bytes.subspan(kMagic.size());
  std::ranges::copy(rest.first<crypto::hash::kSha256Size>(),
                    entry.source_hash.begin());
  base::span<const uint8_t> blob = rest.subspan(crypto::hash::kSha256Size);
  entry.blob.assign(blob.begin(), blob.end());
  return entry;
}

void WriteToDisk(const base::FilePath& path, std::vector<uint8_t> payload) {
  if (path.empty())
    return;
  base::CreateDirectory(path.DirName());
  base::WriteFile(path, payload);
}

}  // namespace

std::string IdForWebPreferencesPreload(const base::FilePath& path) {
  return "preload-" + path.AsUTF8Unsafe();
}

GURL SiteForFrame(content::RenderFrameHost* rfh) {
  return rfh ? rfh->GetSiteInstance()->GetSiteURL() : GURL();
}

std::vector<uint8_t> Get(const std::string& id,
                         const GURL& site,
                         const SourceHash& source_hash) {
  auto& cache = GetMemoryCache();
  const std::string key = CacheKey(id, site);
  auto it = cache.find(key);
  if (it == cache.end()) {
    // Cold lookup: read disk once and memoize (including misses).
    std::optional<Entry> entry;
    std::string file_contents;
    base::FilePath path = PathForEntry(id, site);
    if (!path.empty() && base::ReadFileToString(path, &file_contents))
      entry = ParseDiskEntry(file_contents);
    it = cache.insert_or_assign(it, key, std::move(entry));
  }
  const std::optional<Entry>& entry = it->second;
  if (!entry || entry->source_hash != source_hash)
    return {};
  return entry->blob;
}

void RecordServedPreloads(content::RenderFrameHost* rfh,
                          std::vector<ServedPreload> served) {
  if (!rfh)
    return;
  const content::GlobalRenderFrameHostToken token = rfh->GetGlobalFrameToken();
  if (served.empty()) {
    GetServedPreloads().erase(token);
    return;
  }
  ServedPreloadTracker::Track(rfh);
  GetServedPreloads().insert_or_assign(token, ServedHashes(std::move(served)));
}

void SetFromRenderer(content::RenderFrameHost* rfh,
                     const std::string& id,
                     base::span<const uint8_t> claimed_source_hash,
                     base::span<const uint8_t> blob) {
  if (!rfh || blob.empty())
    return;
  // Only accept writes for a preload id the browser actually served to this
  // exact frame, and only when the renderer's claim about which source it
  // compiled matches the bytes the browser served. The renderer-supplied
  // hash is never persisted — the browser-recorded one is — so a compromised
  // renderer cannot make the browser vouch for bytecode under metadata of
  // its choosing. A mismatch (e.g. a write racing a newer push after the
  // preload changed on disk) just drops the write; the next navigation
  // re-produces the cache.
  const auto& served = GetServedPreloads();
  const auto frame_it = served.find(rfh->GetGlobalFrameToken());
  if (frame_it == served.end())
    return;
  const auto hash_it = frame_it->second.find(id);
  if (hash_it == frame_it->second.end())
    return;
  if (claimed_source_hash.size() != crypto::hash::kSha256Size ||
      !std::ranges::equal(claimed_source_hash, hash_it->second)) {
    return;
  }

  // No first-writer-wins guard: writes must always overwrite. Get() memoizes
  // whatever is on disk *including a corrupt blob* (only V8 in the renderer
  // can validate it), so a renderer that rejected a stale/corrupt cache and
  // re-produced a good one calls SetPreloadCodeCache() to self-heal —
  // skipping that because the memoized entry is "non-empty" would pin the
  // bad cache forever. The duplicate-ship case (two renderers, same preload,
  // first launch) just writes identical bytes twice; harmless and rare, and
  // SetFromRenderer() is UI-thread serialized so it's not a data race.
  Entry entry;
  entry.source_hash = hash_it->second;
  entry.blob.assign(blob.begin(), blob.end());

  std::vector<uint8_t> payload;
  payload.reserve(kMagic.size() + entry.source_hash.size() + entry.blob.size());
  payload.insert(payload.end(), kMagic.begin(), kMagic.end());
  payload.insert(payload.end(), entry.source_hash.begin(),
                 entry.source_hash.end());
  payload.insert(payload.end(), entry.blob.begin(), entry.blob.end());

  const GURL site = SiteForFrame(rfh);
  GetMemoryCache().insert_or_assign(CacheKey(id, site), std::move(entry));
  // Fire-and-forget; a lost write just costs one cold compile next launch.
  // SKIP_ON_SHUTDOWN so an in-flight write finishes (no torn cache file)
  // rather than being abandoned mid-write during teardown.
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&WriteToDisk, PathForEntry(id, site), std::move(payload)));
}

}  // namespace electron::preload_code_cache
