// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ASAR_ARCHIVE_H_
#define ATOM_COMMON_ASAR_ARCHIVE_H_

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

namespace base {
class DictionaryValue;
}

namespace asar {

class Archive : public base::RefCounted<Archive> {
 public:
  struct FileInfo {
    uint32 size;
    uint64 offset;
  };

  explicit Archive(const base::FilePath& path);

  // Read and parse the header.
  bool Init();

  // Get the info of a file.
  bool GetFileInfo(const base::FilePath& path, FileInfo* info);

  base::FilePath path() const { return path_; }
  base::DictionaryValue* header() const { return header_.get(); }

 private:
  friend class base::RefCounted<Archive>;
  virtual ~Archive();

  base::FilePath path_;
  uint32 header_size_;
  scoped_ptr<base::DictionaryValue> header_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

}  // namespace asar

#endif  // ATOM_COMMON_ASAR_ARCHIVE_H_
