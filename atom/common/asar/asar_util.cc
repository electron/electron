// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/asar_util.h"

#include <map>
#include <string>

#include "atom/common/asar/archive.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/stl_util.h"
#include "base/threading/thread_local.h"
#include "base/threading/thread_restrictions.h"

namespace asar {

namespace {

// The global instance of ArchiveMap, will be destroyed on exit.
typedef std::map<base::FilePath, std::shared_ptr<Archive>> ArchiveMap;
base::LazyInstance<base::ThreadLocalPointer<ArchiveMap>>::Leaky
    g_archive_map_tls = LAZY_INSTANCE_INITIALIZER;

const base::FilePath::CharType kAsarExtension[] = FILE_PATH_LITERAL(".asar");

std::map<base::FilePath, bool> g_is_directory_cache;

bool IsDirectoryCached(const base::FilePath& path) {
  auto it = g_is_directory_cache.find(path);
  if (it != g_is_directory_cache.end()) {
    return it->second;
  }
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return g_is_directory_cache[path] = base::DirectoryExists(path);
}

}  // namespace

std::shared_ptr<Archive> GetOrCreateAsarArchive(const base::FilePath& path) {
  if (!g_archive_map_tls.Pointer()->Get())
    g_archive_map_tls.Pointer()->Set(new ArchiveMap);
  ArchiveMap& archive_map = *g_archive_map_tls.Pointer()->Get();
  if (!ContainsKey(archive_map, path)) {
    std::shared_ptr<Archive> archive(new Archive(path));
    if (!archive->Init())
      return nullptr;
    archive_map[path] = archive;
  }
  return archive_map[path];
}

void ClearArchives() {
  if (g_archive_map_tls.Pointer()->Get())
    delete g_archive_map_tls.Pointer()->Get();
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
  return static_cast<int>(info.size) ==
         src.Read(info.offset, const_cast<char*>(contents->data()),
                  contents->size());
}

}  // namespace asar
