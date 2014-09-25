// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/archive_factory.h"

#include "atom/common/asar/archive.h"

namespace asar {

ArchiveFactory::ArchiveFactory() {}

ArchiveFactory::~ArchiveFactory() {
}

Archive* ArchiveFactory::GetOrCreate(const base::FilePath& path) {
  if (!archives_.contains(path)) {
    scoped_ptr<Archive> archive(new Archive(path));
    if (!archive->Init())
      return nullptr;

    archives_.set(path, archive.Pass());
  }

  return archives_.get(path);
}

}  // namespace asar
