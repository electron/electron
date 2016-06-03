// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_ASAR_ARCHIVE_H_
#define ATOM_COMMON_ASAR_ARCHIVE_H_

#include <vector>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"

namespace base {
class DictionaryValue;
}

namespace asar {

class ScopedTemporaryFile;

// This class represents an asar package, and provides methods to read
// information from it.
class Archive {
 public:
  struct FileInfo {
    FileInfo() : unpacked(false), executable(false), size(0), offset(0) {}
    bool unpacked;
    bool executable;
    uint32_t size;
    uint64_t offset;
  };

  struct Stats : public FileInfo {
    Stats() : is_file(true), is_directory(false), is_link(false) {}
    bool is_file;
    bool is_directory;
    bool is_link;
  };

  explicit Archive(const base::FilePath& path);
  virtual ~Archive();

  // Read and parse the header.
  bool Init();

  // Get the info of a file.
  bool GetFileInfo(const base::FilePath& path, FileInfo* info);

  // Fs.stat(path).
  bool Stat(const base::FilePath& path, Stats* stats);

  // Fs.readdir(path).
  bool Readdir(const base::FilePath& path, std::vector<base::FilePath>* files);

  // Fs.realpath(path).
  bool Realpath(const base::FilePath& path, base::FilePath* realpath);

  // Copy the file into a temporary file, and return the new path.
  // For unpacked file, this method will return its real path.
  bool CopyFileOut(const base::FilePath& path, base::FilePath* out);

  // Returns the file's fd.
  int GetFD() const;

  base::FilePath path() const { return path_; }
  base::DictionaryValue* header() const { return header_.get(); }

 private:
  base::FilePath path_;
  base::File file_;
  int fd_;
  uint32_t header_size_;
  std::unique_ptr<base::DictionaryValue> header_;

  // Cached external temporary files.
  base::ScopedPtrHashMap<base::FilePath, std::unique_ptr<ScopedTemporaryFile>>
      external_files_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

}  // namespace asar

#endif  // ATOM_COMMON_ASAR_ARCHIVE_H_
