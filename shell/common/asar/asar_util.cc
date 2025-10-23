// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/asar_util.h"

#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_local.h"
#include "crypto/hash.h"
#include "shell/common/asar/archive.h"
#include "shell/common/thread_restrictions.h"

namespace asar {

namespace {

using ArchiveMap = std::map<base::FilePath, std::shared_ptr<Archive>>;

const base::FilePath::CharType kAsarExtension[] = FILE_PATH_LITERAL(".asar");

bool IsDirectoryCached(const base::FilePath& path) {
  static base::NoDestructor<std::map<base::FilePath, bool>>
      s_is_directory_cache;
  static base::NoDestructor<base::Lock> lock;

  base::AutoLock auto_lock(*lock);
  auto& is_directory_cache = *s_is_directory_cache;

  auto it = is_directory_cache.find(path);
  if (it != is_directory_cache.end()) {
    return it->second;
  }
  electron::ScopedAllowBlockingForElectron allow_blocking;
  return is_directory_cache[path] = base::DirectoryExists(path);
}

ArchiveMap& GetArchiveCache() {
  static base::NoDestructor<ArchiveMap> s_archive_map;
  return *s_archive_map;
}

base::Lock& GetArchiveCacheLock() {
  static base::NoDestructor<base::Lock> lock;
  return *lock;
}

}  // namespace

std::shared_ptr<Archive> GetOrCreateAsarArchive(const base::FilePath& path) {
  base::AutoLock auto_lock(GetArchiveCacheLock());
  ArchiveMap& map = GetArchiveCache();

  // if we have it, return it
  const auto lower = map.lower_bound(path);
  if (lower != std::end(map) && !map.key_comp()(path, lower->first))
    return lower->second;

  // if we can create it, return it
  auto archive = std::make_shared<Archive>(path);
  if (archive->Init()) {
    map.try_emplace(lower, path, archive);
    return archive;
  }

  // didn't have it, couldn't create it
  return nullptr;
}

bool GetAsarArchivePath(const base::FilePath& full_path,
                        base::FilePath* asar_path,
                        base::FilePath* relative_path,
                        bool allow_root) {
  base::FilePath iter = full_path;
  while (true) {
    base::FilePath dirname = iter.DirName();
    if (iter.MatchesExtension(kAsarExtension) && !IsDirectoryCached(iter))
      break;
    else if (iter == dirname)
      return false;
    iter = dirname;
  }

  base::FilePath tail;
  if (!((allow_root && iter == full_path) ||
        iter.AppendRelativePath(full_path, &tail)))
    return false;

  *asar_path = iter;
  *relative_path = tail;
  return true;
}

bool ReadFileToString(const base::FilePath& path, std::string* contents) {
  base::FilePath asar_path, relative_path;
  if (!GetAsarArchivePath(path, &asar_path, &relative_path))
    return base::ReadFileToString(path, contents);

  std::shared_ptr<Archive> archive = GetOrCreateAsarArchive(asar_path);
  if (!archive)
    return false;

  Archive::FileInfo info;
  if (!archive->GetFileInfo(relative_path, &info))
    return false;

  if (info.unpacked) {
    base::FilePath real_path;
    // For unpacked file it will return the real path instead of doing the copy.
    archive->CopyFileOut(relative_path, &real_path);
    return base::ReadFileToString(real_path, contents);
  }

  base::File src(asar_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!src.IsValid())
    return false;

  contents->resize(info.size);
  if (!src.ReadAndCheck(info.offset, base::as_writable_byte_span(*contents)))
    return false;

  if (info.integrity)
    ValidateIntegrityOrDie(base::as_byte_span(*contents), *info.integrity);

  return true;
}

void ValidateIntegrityOrDie(base::span<const uint8_t> input,
                            const IntegrityPayload& integrity) {
  if (integrity.algorithm == HashAlgorithm::kSHA256) {
    const std::string hex_hash =
        base::ToLowerASCII(base::HexEncode(crypto::hash::Sha256(input)));
    if (integrity.hash != hex_hash) {
      LOG(FATAL) << "Integrity check failed for asar archive ("
                 << integrity.hash << " vs " << hex_hash << ")";
    }
  } else {
    LOG(FATAL) << "Unsupported hashing algorithm in ValidateIntegrityOrDie";
  }
}

}  // namespace asar
