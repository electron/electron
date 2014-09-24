// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/asar/archive.h"

#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"

namespace asar {

namespace {

bool GetChildNode(const std::string& name,
                  const base::DictionaryValue* root,
                  const base::DictionaryValue** out) {
  const base::DictionaryValue* files = NULL;
  return root->GetDictionaryWithoutPathExpansion("files", &files) &&
         files->GetDictionaryWithoutPathExpansion(name, out);
}

bool GetNodeFromPath(std::string path,
                     const base::DictionaryValue* root,
                     const base::DictionaryValue** out) {
  if (path == "") {
    *out = root;
    return true;
  }

  for (size_t delimiter_position = path.find('/');
       delimiter_position != std::string::npos;
       delimiter_position = path.find('/')) {
    const base::DictionaryValue* child = NULL;
    if (!GetChildNode(path.substr(0, delimiter_position), root, &child))
      return false;

    root = child;
    path.erase(0, delimiter_position + 1);
  }

  return GetChildNode(path, root, out);
}

bool FillFileInfoWithNode(Archive::FileInfo* info,
                          uint32 header_size,
                          const base::DictionaryValue* node) {
  std::string offset;
  if (!node->GetString("offset", &offset))
    return false;
  if (!base::StringToUint64(offset, &info->offset))
    return false;

  int size;
  if (!node->GetInteger("size", &size))
    return false;

  info->offset += header_size;
  info->size = static_cast<uint32>(size);
  return true;
}

}  // namespace

Archive::Archive(const base::FilePath& path)
    : path_(path),
      header_size_(0) {
}

Archive::~Archive() {
}

bool Archive::Init() {
  base::File file(path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid()) {
    PLOG(ERROR) << "Unable to open " << path_.value();
    return false;
  }

  std::vector<char> buf;
  int len;

  buf.resize(8);
  len = file.ReadAtCurrentPos(buf.data(), buf.size());
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header size from " << path_.value();
    return false;
  }

  uint32 size;
  if (!PickleIterator(Pickle(buf.data(), buf.size())).ReadUInt32(&size)) {
    LOG(ERROR) << "Failed to parse header size from " << path_.value();
    return false;
  }

  buf.resize(size);
  len = file.ReadAtCurrentPos(buf.data(), buf.size());
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header from " << path_.value();
    return false;
  }

  std::string header;
  if (!PickleIterator(Pickle(buf.data(), buf.size())).ReadString(&header)) {
    LOG(ERROR) << "Failed to parse header from " << path_.value();
    return false;
  }

  std::string error;
  JSONStringValueSerializer serializer(&header);
  base::Value* value = serializer.Deserialize(NULL, &error);
  if (!value || !value->IsType(base::Value::TYPE_DICTIONARY)) {
    LOG(ERROR) << "Failed to parse header: " << error;
    return false;
  }

  header_size_ = 8 + size;
  header_.reset(static_cast<base::DictionaryValue*>(value));
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

  if (node->HasKey("link")) {
    stats->is_file = false;
    stats->is_link = true;
    return true;
  }

  if (node->HasKey("files")) {
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
  if (!node->GetDictionaryWithoutPathExpansion("files", &files))
    return false;

  base::DictionaryValue::Iterator iter(*files);
  while (!iter.IsAtEnd()) {
    list->push_back(base::FilePath::FromUTF8Unsafe(iter.key()));
    iter.Advance();
  }
  return true;
}

}  // namespace asar
