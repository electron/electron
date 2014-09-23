// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/archive_factory.h"

#include "atom/common/asar/archive.h"
#include "base/stl_util.h"

namespace asar {

ArchiveFactory::ArchiveFactory() {}

ArchiveFactory::~ArchiveFactory() {}

scoped_refptr<Archive> ArchiveFactory::GetOrCreate(const base::FilePath& path) {
  // Create a cache of Archive.
  if (!ContainsKey(archives_, path)) {
    scoped_refptr<Archive> archive(new Archive(path));
    if (!archive->Init())
      return NULL;
    archives_[path] = archive;
    return archive;
  }

  return archives_[path];
}

}  // namespace asar
