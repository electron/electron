// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/asar_util.h"
#include "net/base/filename_util.h"
#include "url/gurl.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/synchronization/lock.h"
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
  using StringType = base::FilePath::StringType;
  constexpr auto is_separator = &base::FilePath::IsSeparator;
  const StringType& value = full_path.value();

  // Walk the components from the deepest one up, the way repeated DirName()
  // calls would, but on offsets into |value| instead of freshly built paths.
  size_t end = value.size();
  while (end > 0 && is_separator(value[end - 1]))
    --end;
  size_t component_end = end;
  bool leaf = true;
  while (true) {
    size_t start = component_end;
    while (start > 0 && !is_separator(value[start - 1]))
      --start;
    const auto component = base::FilePath::StringViewType{value}.substr(
        start, component_end - start);
    // ".asar" is five characters; MatchesExtension() decides the rest (case
    // folding, dotfiles) exactly as it does for a whole path.
    if (component.size() >= 5 &&
        component[component.size() - 5] ==
            base::FilePath::kExtensionSeparator &&
        base::FilePath(component).MatchesExtension(kAsarExtension)) {
      base::FilePath candidate =
          leaf ? full_path : base::FilePath(value.substr(0, component_end));
      if (!IsDirectoryCached(candidate)) {
        if (leaf && !allow_root)
          return false;
        base::FilePath tail;
        size_t pos = component_end;
        while (pos < end) {
          while (pos < end && is_separator(value[pos]))
            ++pos;
          size_t next = pos;
          while (next < end && !is_separator(value[next]))
            ++next;
          if (next > pos) {
            tail = tail.Append(
                base::FilePath::StringViewType{value}.substr(pos, next - pos));
          }
          pos = next;
        }
        *asar_path = std::move(candidate);
        *relative_path = std::move(tail);
        return true;
      }
    }
    if (start == 0)
      return false;
    leaf = false;
    component_end = start;
    while (component_end > 0 && is_separator(value[component_end - 1]))
      --component_end;
    if (component_end == 0)
      return false;
  }
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

  // Read through the archive's retained file handle rather than re-opening
  // the archive by path: |info|'s offset and integrity hash come from the
  // cached header, and the file on disk may have been replaced (e.g. by an
  // app update) since that header was read. The retained handle always sees
  // the bytes the header describes.
  contents->resize(info.size);
  if (!archive->ReadFileAt(info.offset, base::as_writable_byte_span(*contents)))
    return false;

  if (info.integrity)
    ValidateIntegrityOrDie(base::as_byte_span(*contents), *info.integrity,
                           relative_path.AsUTF8Unsafe());

  return true;
}

void ValidateIntegrityOrDie(base::span<const uint8_t> input,
                            const IntegrityPayload& integrity,
                            std::string_view what) {
  if (integrity.algorithm == HashAlgorithm::kSHA256) {
    const std::string hex_hash =
        base::ToLowerASCII(base::HexEncode(crypto::hash::Sha256(input)));
    if (integrity.hash != hex_hash) {
      LOG(FATAL) << "Integrity check failed for asar archive entry '" << what
                 << "' (" << integrity.hash << " vs " << hex_hash << ", "
                 << input.size() << " bytes)";
    }
  } else {
    LOG(FATAL) << "Unsupported hashing algorithm in ValidateIntegrityOrDie";
  }
}

bool GetExtractedFileURL(const GURL& url,
                         GURL* extracted_url,
                         std::u16string* file_name) {
  base::FilePath path;
  if (!url.SchemeIsFile() || !net::FileURLToFilePath(url, &path))
    return false;
  base::FilePath asar_path, relative_path;
  if (!GetAsarArchivePath(path, &asar_path, &relative_path))
    return false;
  std::shared_ptr<Archive> archive = GetOrCreateAsarArchive(asar_path);
  base::FilePath extracted_path;
  if (!archive || !archive->CopyFileOut(relative_path, &extracted_path))
    return false;
  *extracted_url = net::FilePathToFileURL(extracted_path);
  *file_name = path.BaseName().AsUTF16Unsafe();
  return true;
}

}  // namespace asar
