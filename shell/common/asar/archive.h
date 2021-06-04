// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_ASAR_ARCHIVE_H_
#define SHELL_COMMON_ASAR_ARCHIVE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"

namespace base {
class DictionaryValue;
}

namespace asar {

class ScopedTemporaryFile;

// This class represents an asar package, and provides methods to read
// information from it. It is thread-safe after |Init| has been called.
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
  bool GetFileInfo(const base::FilePath& path, FileInfo* info) const;

  // Fs.stat(path).
  bool Stat(const base::FilePath& path, Stats* stats) const;

  // Fs.readdir(path).
  bool Readdir(const base::FilePath& path,
               std::vector<base::FilePath>* files) const;

  // Fs.realpath(path).
  bool Realpath(const base::FilePath& path, base::FilePath* realpath) const;

  // Copy the file into a temporary file, and return the new path.
  // For unpacked file, this method will return its real path.
  bool CopyFileOut(const base::FilePath& path, base::FilePath* out);

  // Returns the file's fd.
  int GetFD() const;

  base::FilePath path() const { return path_; }

 private:
  bool initialized_;
  const base::FilePath path_;
  base::File file_;
  int fd_ = -1;
  uint32_t header_size_ = 0;
  std::unique_ptr<base::DictionaryValue> header_;

  // Cached external temporary files.
  base::Lock external_files_lock_;
  std::unordered_map<base::FilePath::StringType,
                     std::unique_ptr<ScopedTemporaryFile>>
      external_files_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

}  // namespace asar

#endif  // SHELL_COMMON_ASAR_ARCHIVE_H_
