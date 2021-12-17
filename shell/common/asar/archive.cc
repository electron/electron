// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "electron/fuses.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/asar/scoped_temporary_file.h"

#if defined(OS_WIN)
#include <io.h>
#endif

namespace asar {

namespace {

#if defined(OS_WIN)
const char kSeparators[] = "\\/";
#else
const char kSeparators[] = "/";
#endif

bool GetNodeFromPath(std::string path,
                     const base::DictionaryValue* root,
                     const base::DictionaryValue** out);

// Gets the "files" from "dir".
bool GetFilesNode(const base::DictionaryValue* root,
                  const base::DictionaryValue* dir,
                  const base::DictionaryValue** out) {
  // Test for symbol linked directory.
  const std::string* link = dir->FindStringKey("link");
  if (link != nullptr) {
    const base::DictionaryValue* linked_node = nullptr;
    if (!GetNodeFromPath(*link, root, &linked_node))
      return false;
    dir = linked_node;
  }

  return dir->GetDictionaryWithoutPathExpansion("files", out);
}

// Gets sub-file "name" from "dir".
bool GetChildNode(const base::DictionaryValue* root,
                  const std::string& name,
                  const base::DictionaryValue* dir,
                  const base::DictionaryValue** out) {
  if (name.empty()) {
    *out = root;
    return true;
  }

  const base::DictionaryValue* files = nullptr;
  return GetFilesNode(root, dir, &files) &&
         files->GetDictionaryWithoutPathExpansion(name, out);
}

// Gets the node of "path" from "root".
bool GetNodeFromPath(std::string path,
                     const base::DictionaryValue* root,
                     const base::DictionaryValue** out) {
  if (path.empty()) {
    *out = root;
    return true;
  }

  const base::DictionaryValue* dir = root;
  for (size_t delimiter_position = path.find_first_of(kSeparators);
       delimiter_position != std::string::npos;
       delimiter_position = path.find_first_of(kSeparators)) {
    const base::DictionaryValue* child = nullptr;
    if (!GetChildNode(root, path.substr(0, delimiter_position), dir, &child))
      return false;

    dir = child;
    path.erase(0, delimiter_position + 1);
  }

  return GetChildNode(root, path, dir, out);
}

bool FillFileInfoWithNode(Archive::FileInfo* info,
                          uint32_t header_size,
                          bool load_integrity,
                          const base::DictionaryValue* node) {
  if (auto size = node->FindIntKey("size")) {
    info->size = static_cast<uint32_t>(size.value());
  } else {
    return false;
  }

  if (auto unpacked = node->FindBoolKey("unpacked")) {
    info->unpacked = unpacked.value();
    if (info->unpacked) {
      return true;
    }
  }

  auto* offset = node->FindStringKey("offset");
  if (offset &&
      base::StringToUint64(base::StringPiece(*offset), &info->offset)) {
    info->offset += header_size;
  } else {
    return false;
  }

  if (auto executable = node->FindBoolKey("executable")) {
    info->executable = executable.value();
  }

#if defined(OS_MAC)
  if (load_integrity &&
      electron::fuses::IsEmbeddedAsarIntegrityValidationEnabled()) {
    if (auto* integrity = node->FindDictKey("integrity")) {
      auto* algorithm = integrity->FindStringKey("algorithm");
      auto* hash = integrity->FindStringKey("hash");
      auto block_size = integrity->FindIntKey("blockSize");
      auto* blocks = integrity->FindListKey("blocks");

      if (algorithm && hash && block_size && block_size > 0 && blocks) {
        IntegrityPayload integrity_payload;
        integrity_payload.hash = *hash;
        integrity_payload.block_size =
            static_cast<uint32_t>(block_size.value());
        for (auto& value : blocks->GetList()) {
          if (auto* block = value.GetIfString()) {
            integrity_payload.blocks.push_back(*block);
          } else {
            LOG(FATAL)
                << "Invalid block integrity value for file in ASAR archive";
          }
        }
        if (*algorithm == "SHA256") {
          integrity_payload.algorithm = HashAlgorithm::SHA256;
          info->integrity = std::move(integrity_payload);
        }
      }
    }

    if (!info->integrity.has_value()) {
      LOG(FATAL) << "Failed to read integrity for file in ASAR archive";
      return false;
    }
  }
#endif

  return true;
}

}  // namespace

IntegrityPayload::IntegrityPayload()
    : algorithm(HashAlgorithm::NONE), block_size(0) {}
IntegrityPayload::~IntegrityPayload() = default;
IntegrityPayload::IntegrityPayload(const IntegrityPayload& other) = default;

Archive::FileInfo::FileInfo()
    : unpacked(false), executable(false), size(0), offset(0) {}
Archive::FileInfo::~FileInfo() = default;

Archive::Archive(const base::FilePath& path)
    : initialized_(false), path_(path), file_(base::File::FILE_OK) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  file_.Initialize(path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
#if defined(OS_WIN)
  fd_ = _open_osfhandle(reinterpret_cast<intptr_t>(file_.GetPlatformFile()), 0);
#elif defined(OS_POSIX)
  fd_ = file_.GetPlatformFile();
#endif
}

Archive::~Archive() {
#if defined(OS_WIN)
  if (fd_ != -1) {
    _close(fd_);
    // Don't close the handle since we already closed the fd.
    file_.TakePlatformFile();
  }
#endif
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  file_.Close();
}

bool Archive::Init() {
  // Should only be initialized once
  CHECK(!initialized_);
  initialized_ = true;

  if (!file_.IsValid()) {
    if (file_.error_details() != base::File::FILE_ERROR_NOT_FOUND) {
      LOG(WARNING) << "Opening " << path_.value() << ": "
                   << base::File::ErrorToString(file_.error_details());
    }
    return false;
  }

  std::vector<char> buf;
  int len;

  buf.resize(8);
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    len = file_.ReadAtCurrentPos(buf.data(), buf.size());
  }
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header size from " << path_.value();
    return false;
  }

  uint32_t size;
  if (!base::PickleIterator(base::Pickle(buf.data(), buf.size()))
           .ReadUInt32(&size)) {
    LOG(ERROR) << "Failed to parse header size from " << path_.value();
    return false;
  }

  buf.resize(size);
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    len = file_.ReadAtCurrentPos(buf.data(), buf.size());
  }
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header from " << path_.value();
    return false;
  }

  std::string header;
  if (!base::PickleIterator(base::Pickle(buf.data(), buf.size()))
           .ReadString(&header)) {
    LOG(ERROR) << "Failed to parse header from " << path_.value();
    return false;
  }

#if defined(OS_MAC)
  // Validate header signature if required and possible
  if (electron::fuses::IsEmbeddedAsarIntegrityValidationEnabled() &&
      RelativePath().has_value()) {
    absl::optional<IntegrityPayload> integrity = HeaderIntegrity();
    if (!integrity.has_value()) {
      LOG(FATAL) << "Failed to get integrity for validatable asar archive: "
                 << RelativePath().value();
      return false;
    }

    // Currently we only support the sha256 algorithm, we can add support for
    // more below ensure we read them in preference order from most secure to
    // least
    if (integrity.value().algorithm != HashAlgorithm::NONE) {
      ValidateIntegrityOrDie(header.c_str(), header.length(),
                             integrity.value());
    } else {
      LOG(FATAL) << "No eligible hash for validatable asar archive: "
                 << RelativePath().value();
    }

    header_validated_ = true;
  }
#endif

  absl::optional<base::Value> value = base::JSONReader::Read(header);
  if (!value || !value->is_dict()) {
    LOG(ERROR) << "Failed to parse header";
    return false;
  }

  header_size_ = 8 + size;
  header_ = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(*value)));
  return true;
}

#if !defined(OS_MAC)
absl::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  return absl::nullopt;
}

absl::optional<base::FilePath> Archive::RelativePath() const {
  return absl::nullopt;
}
#endif

bool Archive::GetFileInfo(const base::FilePath& path, FileInfo* info) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  std::string link;
  if (node->GetString("link", &link))
    return GetFileInfo(base::FilePath::FromUTF8Unsafe(link), info);

  return FillFileInfoWithNode(info, header_size_, header_validated_, node);
}

bool Archive::Stat(const base::FilePath& path, Stats* stats) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  if (node->FindKey("link")) {
    stats->is_file = false;
    stats->is_link = true;
    return true;
  }

  if (node->FindKey("files")) {
    stats->is_file = false;
    stats->is_directory = true;
    return true;
  }

  return FillFileInfoWithNode(stats, header_size_, header_validated_, node);
}

bool Archive::Readdir(const base::FilePath& path,
                      std::vector<base::FilePath>* files) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  const base::DictionaryValue* files_node;
  if (!GetFilesNode(header_.get(), node, &files_node))
    return false;

  base::DictionaryValue::Iterator iter(*files_node);
  while (!iter.IsAtEnd()) {
    files->push_back(base::FilePath::FromUTF8Unsafe(iter.key()));
    iter.Advance();
  }
  return true;
}

bool Archive::Realpath(const base::FilePath& path,
                       base::FilePath* realpath) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  std::string link;
  if (node->GetString("link", &link)) {
    *realpath = base::FilePath::FromUTF8Unsafe(link);
    return true;
  }

  *realpath = path;
  return true;
}

bool Archive::CopyFileOut(const base::FilePath& path, base::FilePath* out) {
  if (!header_)
    return false;

  base::AutoLock auto_lock(external_files_lock_);

  auto it = external_files_.find(path.value());
  if (it != external_files_.end()) {
    *out = it->second->path();
    return true;
  }

  FileInfo info;
  if (!GetFileInfo(path, &info))
    return false;

  if (info.unpacked) {
    *out = path_.AddExtension(FILE_PATH_LITERAL("unpacked")).Append(path);
    return true;
  }

  auto temp_file = std::make_unique<ScopedTemporaryFile>();
  base::FilePath::StringType ext = path.Extension();
  if (!temp_file->InitFromFile(&file_, ext, info.offset, info.size,
                               info.integrity))
    return false;

#if defined(OS_POSIX)
  if (info.executable) {
    // chmod a+x temp_file;
    base::SetPosixFilePermissions(temp_file->path(), 0755);
  }
#endif

  *out = temp_file->path();
  external_files_[path.value()] = std::move(temp_file);
  return true;
}

int Archive::GetUnsafeFD() const {
  return fd_;
}

}  // namespace asar
