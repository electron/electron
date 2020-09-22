// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/scoped_temporary_file.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"

namespace asar {

ScopedTemporaryFile::ScopedTemporaryFile() = default;

ScopedTemporaryFile::~ScopedTemporaryFile() {
  if (!path_.empty()) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    // On Windows it is very likely the file is already in use (because it is
    // mostly used for Node native modules), so deleting it now will halt the
    // program.
#if defined(OS_WIN)
    base::DeleteFileAfterReboot(path_);
#else
    base::DeleteFile(path_);
#endif
  }
}

bool ScopedTemporaryFile::Init(const base::FilePath::StringType& ext) {
  if (!path_.empty())
    return true;

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!base::CreateTemporaryFile(&path_))
    return false;

#if defined(OS_WIN)
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

}  // namespace asar
