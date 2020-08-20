// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <string>
#include <utility>
#include <vector>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
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
  std::string link;
  if (dir->GetStringWithoutPathExpansion("link", &link)) {
    const base::DictionaryValue* linked_node = nullptr;
    if (!GetNodeFromPath(link, root, &linked_node))
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
  if (name == "") {
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
  if (path == "") {
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
                          const base::DictionaryValue* node) {
  int size;
  if (!node->GetInteger("size", &size))
    return false;
  info->size = static_cast<uint32_t>(size);

  if (node->GetBoolean("unpacked", &info->unpacked) && info->unpacked)
    return true;

  std::string offset;
  if (!node->GetString("offset", &offset))
    return false;
  if (!base::StringToUint64(offset, &info->offset))
    return false;
  info->offset += header_size;

  node->GetBoolean("executable", &info->executable);

  return true;
}

}  // namespace

Archive::Archive(const base::FilePath& path) : path_(path) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (base::PathExists(path_) && !file_.Initialize(path_)) {
    LOG(ERROR) << "Failed to open ASAR archive at '" << path_.value() << "'";
  }
}

Archive::~Archive() {}

bool Archive::Init() {
  if (!file_.IsValid()) {
    return false;
  }

  if (file_.length() < 8) {
    LOG(ERROR) << "Malformed ASAR file at '" << path_.value()
               << "' (too short)";
    return false;
  }

  uint32_t size;
  base::PickleIterator size_pickle(
      base::Pickle(reinterpret_cast<const char*>(file_.data()), 8));
  if (!size_pickle.ReadUInt32(&size)) {
    LOG(ERROR) << "Failed to read header size at '" << path_.value() << "'";
    return false;
  }

  if (file_.length() - 8 < size) {
    LOG(ERROR) << "Malformed ASAR file at '" << path_.value()
               << "' (incorrect header)";
    return false;
  }

  base::PickleIterator header_pickle(
      base::Pickle(reinterpret_cast<const char*>(file_.data() + 8), size));
  std::string header;
  if (!header_pickle.ReadString(&header)) {
    LOG(ERROR) << "Failed to read header string at '" << path_.value() << "'";
    return false;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(header);
  if (!value || !value->is_dict()) {
    LOG(ERROR) << "Header was not valid JSON at '" << path_.value() << "'";
    return false;
  }

  header_size_ = 8 + size;
  header_ = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(*value)));
  return true;
}

bool Archive::GetFileInfo(const base::FilePath& path, FileInfo* info) {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  std::string link;
  if (node->GetString("link", &link))
    return GetFileInfo(base::FilePath::FromUTF8Unsafe(link), info);

  return FillFileInfoWithNode(info, header_size_, node);
}

bool Archive::Stat(const base::FilePath& path, Stats* stats) {
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

  return FillFileInfoWithNode(stats, header_size_, node);
}

bool Archive::Readdir(const base::FilePath& path,
                      std::vector<base::FilePath>* list) {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  const base::DictionaryValue* files;
  if (!GetFilesNode(header_.get(), node, &files))
    return false;

  base::DictionaryValue::Iterator iter(*files);
  while (!iter.IsAtEnd()) {
    list->push_back(base::FilePath::FromUTF8Unsafe(iter.key()));
    iter.Advance();
  }
  return true;
}

bool Archive::Realpath(const base::FilePath& path, base::FilePath* realpath) {
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

  base::CheckedNumeric<uint64_t> safe_offset(info.offset);
  auto safe_end = safe_offset + info.size;
  if (!safe_end.IsValid() || safe_end.ValueOrDie() > file_.length())
    return false;

  auto temp_file = std::make_unique<ScopedTemporaryFile>();
  base::FilePath::StringType ext = path.Extension();
  if (!temp_file->Init(ext))
    return false;

  base::File dest(temp_file->path(),
                  base::File::FLAG_OPEN | base::File::FLAG_WRITE);
  if (!dest.IsValid())
    return false;

  dest.WriteAtCurrentPos(
      reinterpret_cast<const char*>(file_.data() + info.offset), info.size);

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

}  // namespace asar
