// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "base/files/file_path.h"

namespace base {
class File;
}

namespace asar {

// An object representing a temporary file that should be cleaned up when this
// object goes out of scope.  Note that since deletion occurs during the
// destructor, no further error handling is possible if the directory fails to
// be deleted.  As a result, deletion is not guaranteed by this class.
class ScopedTemporaryFile {
 public:
  ScopedTemporaryFile();
  virtual ~ScopedTemporaryFile();

  // Init an empty temporary file with a certain extension.
  bool Init(const base::FilePath::StringType& ext);

  // Init an temporary file and fill it with content of |path|.
  bool InitFromFile(base::File* src,
                    const base::FilePath::StringType& ext,
                    uint64_t offset, uint64_t size);

  base::FilePath path() const { return path_; }

 private:
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTemporaryFile);
};

}  // namespace asar
