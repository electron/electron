// Copyright 2023 Slack Technologies, Inc.
// Contributors: Weiyun Dai (https://github.com/WeiyunD/), Andrew Lay
// (https://github.com/guohaolay) Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <algorithm>
#include <sstream>

#include "base/base_paths.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/common/asar/asar_util.h"

namespace asar {

const wchar_t kIntegrityCheckResourceType[] = L"Integrity";
const wchar_t kIntegrityCheckResourceItem[] = L"ElectronAsar";

std::optional<base::FilePath> Archive::RelativePath() const {
  base::FilePath exe_path;
  if (!base::PathService::Get(base::FILE_EXE, &exe_path)) {
    LOG(FATAL) << "Couldn't get exe file path";
  }

  base::FilePath relative_path;
  if (!exe_path.DirName().AppendRelativePath(path_, &relative_path)) {
    return std::nullopt;
  }

  return relative_path;
}

std::optional<std::unordered_map<std::string, IntegrityPayload>>
LoadIntegrityConfigCache() {
  static base::NoDestructor<
      std::optional<std::unordered_map<std::string, IntegrityPayload>>>
      integrity_config_cache;

  // Skip loading if cache is already loaded
  if (integrity_config_cache->has_value()) {
    return *integrity_config_cache;
  }

  // Init cache
  *integrity_config_cache = std::unordered_map<std::string, IntegrityPayload>();

  // Load integrity config from exe resource
  HMODULE module_handle = ::GetModuleHandle(NULL);

  HRSRC resource = ::FindResource(module_handle, kIntegrityCheckResourceItem,
                                  kIntegrityCheckResourceType);
  if (!resource) {
    PLOG(FATAL) << "FindResource failed.";
  }

  HGLOBAL rcData = ::LoadResource(module_handle, resource);
  if (!rcData) {
    PLOG(FATAL) << "LoadResource failed.";
  }

  auto* res_data = static_cast<const char*>(::LockResource(rcData));
  int res_size = SizeofResource(module_handle, resource);

  if (!res_data) {
    PLOG(FATAL) << "Failed to integrity config from exe resource.";
  }

  if (!res_size) {
    PLOG(FATAL) << "Unexpected empty integrity config from exe resource.";
  }

  // Parse integrity config payload
  std::string integrity_config_payload = std::string(res_data, res_size);
  std::optional<base::Value> root =
      base::JSONReader::Read(integrity_config_payload);

  if (!root.has_value()) {
    LOG(FATAL) << "Invalid integrity config: NOT a valid JSON.";
  }

  const base::Value::List* file_configs = root.value().GetIfList();
  if (!file_configs) {
    LOG(FATAL) << "Invalid integrity config: NOT a list.";
  }

  // Parse each individual file integrity config
  for (size_t i = 0; i < file_configs->size(); i++) {
    // Skip invalid file configs
    const base::Value::Dict* ele_dict = (*file_configs)[i].GetIfDict();
    if (!ele_dict) {
      LOG(WARNING) << "Skip config " << i << ": NOT a valid dict";
      continue;
    }

    const std::string* file = ele_dict->FindString("file");
    if (!file || file->empty()) {
      LOG(WARNING) << "Skip config " << i << ": Invalid file";
      continue;
    }

    const std::string* alg = ele_dict->FindString("alg");
    if (!alg || base::ToLowerASCII(*alg) != "sha256") {
      LOG(WARNING) << "Skip config " << i << ": Invalid alg";
      continue;
    }

    const std::string* value = ele_dict->FindString("value");
    if (!value || value->empty()) {
      LOG(WARNING) << "Skip config " << i << ": Invalid hash value";
      continue;
    }

    // Add valid file config into cache
    IntegrityPayload header_integrity;
    header_integrity.algorithm = HashAlgorithm::kSHA256;
    header_integrity.hash = base::ToLowerASCII(*value);

    integrity_config_cache->value()[base::ToLowerASCII(*file)] =
        std::move(header_integrity);
  }

  return *integrity_config_cache;
}

std::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  std::optional<base::FilePath> relative_path = RelativePath();
  // Callers should have already asserted this
  CHECK(relative_path.has_value());

  // Load integrity config from exe resource
  std::optional<std::unordered_map<std::string, IntegrityPayload>>
      integrity_config = LoadIntegrityConfigCache();
  if (!integrity_config.has_value()) {
    LOG(WARNING) << "Failed to integrity config from exe resource.";
    return std::nullopt;
  }

  // Convert Window rel path to UTF8 lower case
  std::string rel_path_utf8 = base::WideToUTF8(relative_path.value().value());
  rel_path_utf8 = base::ToLowerASCII(rel_path_utf8);

  // Find file integrity config
  auto iter = integrity_config.value().find(rel_path_utf8);
  if (iter == integrity_config.value().end()) {
    LOG(FATAL) << "Failed to find file integrity info for " << rel_path_utf8;
  }

  return iter->second;
}

}  // namespace asar
