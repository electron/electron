// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ASAR_ARCHIVE_FACTORY_H_
#define ATOM_COMMON_ASAR_ARCHIVE_FACTORY_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/files/file_path.h"

namespace asar {

class Archive;

class ArchiveFactory {
 public:
  ArchiveFactory();
  virtual ~ArchiveFactory();

  Archive* GetOrCreate(const base::FilePath& path);

 private:
  base::ScopedPtrHashMap<base::FilePath, Archive> archives_;

  DISALLOW_COPY_AND_ASSIGN(ArchiveFactory);
};

}  // namespace asar

#endif  // ATOM_COMMON_ASAR_ARCHIVE_FACTORY_H_
