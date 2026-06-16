// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/preload_code_cache.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/numerics/byte_conversions.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "crypto/hash.h"
#include "url/gurl.h"

namespace electron::preload_code_cache {

namespace {

// Disk format: kMagic + u32le site length + producer site + sha256(source) +
// blob. Older-format files (EPC1, no site) fail the magic check and read as
// a miss.
constexpr std::array<uint8_t, 4> kMagic{'E', 'P', 'C', '2'};

struct Entry {
  std::string site;
  std::array<uint8_t, crypto::hash::kSha256Size> source_hash;
  std::vector<uint8_t> blob;
};

// In-memory tier. The optional<> memoizes a disk miss so it isn't re-tried
// per navigation. UI-thread only: both Get() (from the navigation push paths)
// and SetFromRenderer() (from the ElectronWebContentsUtility associated
// receiver on the RenderFrameHost) are dispatched on the UI thread, so the
// map needs no lock and concurrent SetPreloadCodeCache messages can't race
// it.
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
using ServedHashes =
    base::flat_map<std::string, std::array<uint8_t, crypto::hash::kSha256Size>>;
using ServedPreloads =
    base::flat_map<content::GlobalRenderFrameHostToken, ServedHashes>;
ServedPreloads& GetServedPreloads() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  static base::NoDestructor<ServedPreloads> served;
  return *served;
}

base::FilePath PathForId(const std::string& id) {
  base::FilePath dir;
  if (!base::PathService::Get(chrome::DIR_USER_DATA, &dir))
    return {};
  // sha256(id) keeps the filename filesystem-safe and bounded.
  std::string digest = base::HexEncode(crypto::hash::Sha256(id));
  return dir.AppendASCII("Code Cache")
      .AppendASCII("electron-preload")
      .AppendASCII(digest + ".cache");
}

std::optional<Entry> ParseDiskEntry(const std::string& file_contents) {
  base::span<const uint8_t> bytes = base::as_byte_span(file_contents);
  if (bytes.size() <= kMagic.size() + 4u)
    return std::nullopt;
  if (bytes.first<kMagic.size()>() != base::span(kMagic))
    return std::nullopt;
  base::span<const uint8_t> rest = bytes.subspan(kMagic.size());
  const uint32_t site_size = base::U32FromLittleEndian(rest.first<4u>());
  rest = rest.subspan(4u);
  if (rest.size() < site_size)
    return std::nullopt;
  Entry entry;
  base::span<const uint8_t> site = rest.first(site_size);
  entry.site.assign(site.begin(), site.end());
  rest = rest.subspan(site_size);
  if (rest.size() <= crypto::hash::kSha256Size)
    return std::nullopt;
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

// Writes |entry| to the in-memory tier and (fire-and-forget) to disk.
void StoreEntry(const std::string& id, Entry entry) {
  std::vector<uint8_t> payload;
  const std::array<uint8_t, 4u> site_size =
      base::U32ToLittleEndian(base::checked_cast<uint32_t>(entry.site.size()));
  payload.reserve(kMagic.size() + site_size.size() + entry.site.size() +
                  entry.source_hash.size() + entry.blob.size());
  payload.insert(payload.end(), kMagic.begin(), kMagic.end());
  payload.insert(payload.end(), site_size.begin(), site_size.end());
  payload.insert(payload.end(), entry.site.begin(), entry.site.end());
  payload.insert(payload.end(), entry.source_hash.begin(),
                 entry.source_hash.end());
  payload.insert(payload.end(), entry.blob.begin(), entry.blob.end());

  GetMemoryCache().insert_or_assign(id, std::move(entry));
  // Fire-and-forget; a lost write just costs one cold compile next launch.
  // SKIP_ON_SHUTDOWN so an in-flight write finishes (no torn cache file)
  // rather than being abandoned mid-write during teardown.
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&WriteToDisk, PathForId(id), std::move(payload)));
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
                         base::span<const uint8_t> contents) {
  auto& cache = GetMemoryCache();
  auto it = cache.find(id);
  if (it == cache.end()) {
    // Cold lookup: read disk once and memoize (including misses).
    std::optional<Entry> entry;
    std::string file_contents;
    base::FilePath path = PathForId(id);
    if (!path.empty() && base::ReadFileToString(path, &file_contents))
      entry = ParseDiskEntry(file_contents);
    it = cache.insert_or_assign(it, id, std::move(entry));
  }
  const std::optional<Entry>& entry = it->second;
  if (!entry || entry->site != site.possibly_invalid_spec() ||
      entry->source_hash != crypto::hash::Sha256(contents)) {
    return {};
  }
  return entry->blob;
}

void RecordServedPreloads(
    const content::GlobalRenderFrameHostToken& frame_token,
    const std::vector<mojom::PreloadScriptDataPtr>& scripts) {
  ServedHashes hashes;
  for (const auto& ps : scripts) {
    if (ps->contents.empty())
      continue;
    hashes.insert_or_assign(ps->id, crypto::hash::Sha256(ps->contents));
  }
  auto& served = GetServedPreloads();
  if (hashes.empty())
    served.erase(frame_token);
  else
    served.insert_or_assign(frame_token, std::move(hashes));
}

void OnRenderFrameDeleted(
    const content::GlobalRenderFrameHostToken& frame_token) {
  GetServedPreloads().erase(frame_token);
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
  entry.site = SiteForFrame(rfh).possibly_invalid_spec();
  entry.source_hash = hash_it->second;
  entry.blob.assign(blob.begin(), blob.end());
  StoreEntry(id, std::move(entry));
}

}  // namespace electron::preload_code_cache
