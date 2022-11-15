// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/scoped_temporary_file.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/thread_restrictions.h"

namespace asar {

ScopedTemporaryFile::ScopedTemporaryFile() = default;

ScopedTemporaryFile::~ScopedTemporaryFile() {
  if (!path_.empty()) {
    electron::ScopedAllowBlockingForElectron allow_blocking;
    // On Windows it is very likely the file is already in use (because it is
    // mostly used for Node native modules), so deleting it now will halt the
    // program.
#if BUILDFLAG(IS_WIN)
    base::DeleteFileAfterReboot(path_);
#else
    base::DeleteFile(path_);
#endif
  }
}

bool ScopedTemporaryFile::Init(const base::FilePath::StringType& ext) {
  if (!path_.empty())
    return true;

  electron::ScopedAllowBlockingForElectron allow_blocking;
  if (!base::CreateTemporaryFile(&path_))
    return false;

#if BUILDFLAG(IS_WIN)
  // Keep the original extension.
  if (!ext.empty()) {
    base::FilePath new_path = path_.AddExtension(ext);
    if (!base::Move(path_, new_path))
      return false;
    path_ = new_path;
  }
#endif

  return true;
}

bool ScopedTemporaryFile::InitFromFile(
    base::File* src,
    const base::FilePath::StringType& ext,
    uint64_t offset,
    uint64_t size,
    const absl::optional<IntegrityPayload>& integrity) {
  if (!src->IsValid())
    return false;

  if (!Init(ext))
    return false;

  electron::ScopedAllowBlockingForElectron allow_blocking;
  std::vector<char> buf(size);
  int len = src->Read(offset, buf.data(), buf.size());
  if (len != static_cast<int>(size))
    return false;

  if (integrity.has_value()) {
    ValidateIntegrityOrDie(buf.data(), buf.size(), integrity.value());
  }

  base::File dest(path_, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!dest.IsValid())
    return false;

  return dest.WriteAtCurrentPos(buf.data(), buf.size()) ==
         static_cast<int>(size);
}

}  // namespace asar
