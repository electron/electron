// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/preload_code_cache.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/thread_pool.h"
#include "chrome/common/chrome_paths.h"
#include "crypto/hash.h"

namespace electron::preload_code_cache {

namespace {

// In-memory tier (UI thread only). The optional<> memoizes a disk miss so
// it isn't re-tried per navigation.
using Cache = base::flat_map<std::string, std::optional<std::vector<uint8_t>>>;
Cache& GetMemoryCache() {
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

void WriteToDisk(const base::FilePath& path, std::vector<uint8_t> blob) {
  if (path.empty())
    return;
  base::CreateDirectory(path.DirName());
  base::WriteFile(path, blob);
}

}  // namespace

std::vector<uint8_t> Get(const std::string& id) {
  auto& cache = GetMemoryCache();
  auto it = cache.find(id);
  if (it != cache.end())
    return it->second.value_or(std::vector<uint8_t>());

  // Cold lookup: read disk once and memoize (including misses).
  std::optional<std::vector<uint8_t>> entry;
  std::string contents;
  base::FilePath path = PathForId(id);
  if (!path.empty() && base::ReadFileToString(path, &contents) &&
      !contents.empty()) {
    entry = std::vector<uint8_t>(contents.begin(), contents.end());
  }
  std::vector<uint8_t> result = entry.value_or(std::vector<uint8_t>());
  cache.insert_or_assign(id, std::move(entry));
  return result;
}

void Set(const std::string& id, base::span<const uint8_t> blob) {
  if (blob.empty())
    return;
  std::vector<uint8_t> data(blob.begin(), blob.end());
  GetMemoryCache().insert_or_assign(id, data);
  // Fire-and-forget; a lost write just costs one cold compile next launch.
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(&WriteToDisk, PathForId(id), std::move(data)));
}

}  // namespace electron::preload_code_cache
