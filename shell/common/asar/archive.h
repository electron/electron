// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_COMMON_ASAR_ARCHIVE_H_
#define ELECTRON_SHELL_COMMON_ASAR_ARCHIVE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <uv.h>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/synchronization/lock.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace asar {

class ScopedTemporaryFile;

enum class HashAlgorithm {
  kSHA256,
  kNone,
};

struct IntegrityPayload {
  IntegrityPayload();
  ~IntegrityPayload();
  IntegrityPayload(const IntegrityPayload& other);
  HashAlgorithm algorithm;
  std::string hash;
  uint32_t block_size;
  std::vector<std::string> blocks;
};

// This class represents an asar package, and provides methods to read
// information from it. It is thread-safe after |Init| has been called.
class Archive {
 public:
  struct FileInfo {
    FileInfo();
    ~FileInfo();
    bool unpacked;
    bool executable;
    uint32_t size;
    uint64_t offset;
    absl::optional<IntegrityPayload> integrity;
  };

  enum class FileType {
    kFile = UV_DIRENT_FILE,
    kDirectory = UV_DIRENT_DIR,
    kLink = UV_DIRENT_LINK,
  };

  struct Stats : public FileInfo {
    FileType type = FileType::kFile;
  };

  explicit Archive(const base::FilePath& path);
  virtual ~Archive();

  // disable copy
  Archive(const Archive&) = delete;
  Archive& operator=(const Archive&) = delete;

  // Read and parse the header.
  bool Init();

  absl::optional<IntegrityPayload> HeaderIntegrity() const;
  absl::optional<base::FilePath> RelativePath() const;

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
  // Using this fd will not validate the integrity of any files
  // you read out of the ASAR manually.  Callers are responsible
  // for integrity validation after this fd is handed over.
  int GetUnsafeFD() const;

  base::FilePath path() const { return path_; }

 private:
  bool initialized_;
  bool header_validated_ = false;
  const base::FilePath path_;
  base::File file_;
  int fd_ = -1;
  uint32_t header_size_ = 0;
  absl::optional<base::Value::Dict> header_;

  // Cached external temporary files.
  base::Lock external_files_lock_;
  std::unordered_map<base::FilePath::StringType,
                     std::unique_ptr<ScopedTemporaryFile>>
      external_files_;
};

}  // namespace asar

#endif  // ELECTRON_SHELL_COMMON_ASAR_ARCHIVE_H_
