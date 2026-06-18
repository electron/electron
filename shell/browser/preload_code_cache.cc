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
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/hash.h"

namespace electron::preload_code_cache {

namespace {

// Disk format: kMagic + sha256(source) + blob. Pre-hash-format files fail
// the magic check and read as a miss.
constexpr std::array<uint8_t, 4> kMagic{'E', 'P', 'C', '1'};

struct Entry {
  std::array<uint8_t, crypto::hash::kSha256Size> source_hash;
  std::vector<uint8_t> blob;
};

// In-memory tier. The optional<> memoizes a disk miss so it isn't re-tried
// per navigation. UI-thread only: both Get() (from the navigation push paths)
// and Set() (from the ElectronWebContentsUtility associated receiver on the
// RenderFrameHost) are dispatched on the UI thread, so the map needs no lock
// and concurrent SetPreloadCodeCache messages can't race it.
using Cache = base::flat_map<std::string, std::optional<Entry>>;
Cache& GetMemoryCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  static base::NoDestructor<Cache> cache;
  return *cache;
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

std::vector<uint8_t> Get(const std::string& id,
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
  if (!entry || entry->source_hash != crypto::hash::Sha256(contents))
    return {};
  return entry->blob;
}

void Set(const std::string& id,
         base::span<const uint8_t> source_hash,
         base::span<const uint8_t> blob) {
  if (blob.empty() || source_hash.size() != crypto::hash::kSha256Size)
    return;
  // No first-writer-wins guard: Set() must always overwrite. Get() memoizes
  // whatever is on disk *including a corrupt blob* (only V8 in the renderer
  // can validate it), so a renderer that rejected a stale/corrupt cache and
  // re-produced a good one calls Set() to self-heal — skipping that because
  // the memoized entry is "non-empty" would pin the bad cache forever. The
  // duplicate-ship case (two renderers, same preload, first launch) just
  // writes identical bytes twice; harmless and rare, and Set() is UI-thread
  // serialized so it's not a data race.
  Entry entry;
  std::ranges::copy(source_hash, entry.source_hash.begin());
  entry.blob.assign(blob.begin(), blob.end());

  std::vector<uint8_t> payload;
  payload.reserve(kMagic.size() + source_hash.size() + blob.size());
  payload.insert(payload.end(), kMagic.begin(), kMagic.end());
  payload.insert(payload.end(), source_hash.begin(), source_hash.end());
  payload.insert(payload.end(), blob.begin(), blob.end());

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

}  // namespace electron::preload_code_cache
