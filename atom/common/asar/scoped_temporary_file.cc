// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/scoped_temporary_file.h"

#include <vector>

#include "base/files/file_util.h"
#include "base/threading/thread_restrictions.h"

namespace asar {

ScopedTemporaryFile::ScopedTemporaryFile() {
}

ScopedTemporaryFile::~ScopedTemporaryFile() {
  if (!path_.empty()) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    // On Windows it is very likely the file is already in use (because it is
    // mostly used for Node native modules), so deleting it now will halt the
    // program.
#if defined(OS_WIN)
    base::DeleteFileAfterReboot(path_);
#else
    base::DeleteFile(path_, false);
#endif
  }
}

bool ScopedTemporaryFile::Init() {
  if (!path_.empty())
    return true;

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  return base::CreateTemporaryFile(&path_);
}

bool ScopedTemporaryFile::InitFromFile(base::File* src,
                                       uint64 offset, uint64 size) {
  if (!src->IsValid())
    return false;

  if (!Init())
    return false;

  std::vector<char> buf(size);
  int len = src->Read(offset, buf.data(), buf.size());
  if (len != static_cast<int>(size))
    return false;

  base::File dest(path_, base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!dest.IsValid())
    return false;

  return dest.WriteAtCurrentPos(buf.data(), buf.size()) ==
      static_cast<int>(size);
}

}  // namespace asar
