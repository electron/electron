// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_
#define ATOM_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_

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

  // Init an empty temporary file.
  bool Init();

  // Init an temporary file and fill it with content of |path|.
  bool InitFromFile(base::File* src, uint64 offset, uint64 size);

  base::FilePath path() const { return path_; }

 private:
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTemporaryFile);
};

}  // namespace asar

#endif  // ATOM_COMMON_ASAR_SCOPED_TEMPORARY_FILE_H_
