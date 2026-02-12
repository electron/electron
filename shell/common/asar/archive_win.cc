// Copyright 2023 Slack Technologies, Inc.
// Contributors: Weiyun Dai (https://github.com/WeiyunD/), Andrew Lay
// (https://github.com/guohaolay) Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <algorithm>
#include <string_view>

#include "base/base_paths.h"
#include "base/containers/map_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/common/asar/asar_util.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace asar {

const wchar_t kIntegrityCheckResourceType[] = L"Integrity";
const wchar_t kIntegrityCheckResourceItem[] = L"ElectronAsar";

std::optional<base::FilePath> Archive::RelativePath() const {
  base::FilePath assets_dir;
  if (!base::PathService::Get(base::DIR_ASSETS, &assets_dir)) {
    LOG(FATAL) << "Couldn't get assets directory path";
  }

  base::FilePath relative_path;
  if (!assets_dir.AppendRelativePath(path_, &relative_path)) {
    return std::nullopt;
  }

  return relative_path;
}

namespace {

auto LoadIntegrityConfig() {
  absl::flat_hash_map<std::string, IntegrityPayload> cache;

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

  const auto* res_data = static_cast<const char*>(::LockResource(rcData));
  const auto res_size = SizeofResource(module_handle, resource);

  if (!res_data) {
    PLOG(FATAL) << "Failed to integrity config from exe resource.";
  }

  if (!res_size) {
    PLOG(FATAL) << "Unexpected empty integrity config from exe resource.";
  }

  // Parse integrity config payload
  std::optional<base::Value> root =
      base::JSONReader::Read(std::string_view{res_data, res_size},
                             base::JSON_PARSE_CHROMIUM_EXTENSIONS);

  if (!root.has_value()) {
    LOG(FATAL) << "Invalid integrity config: NOT a valid JSON.";
  }

  const base::ListValue* file_configs = root.value().GetIfList();
  if (!file_configs) {
    LOG(FATAL) << "Invalid integrity config: NOT a list.";
  }

  // Parse each individual file integrity config
  cache.reserve(file_configs->size());
  for (size_t i = 0; i < file_configs->size(); i++) {
    // Skip invalid file configs
    const base::DictValue* ele_dict = (*file_configs)[i].GetIfDict();
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

    cache.insert_or_assign(base::ToLowerASCII(*file),
                           std::move(header_integrity));
  }

  return cache;
}

const auto& GetIntegrityConfigCache() {
  static const auto cache = base::NoDestructor(LoadIntegrityConfig());
  return *cache;
}

}  // namespace

std::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  const std::optional<base::FilePath> relative_path = RelativePath();
  CHECK(relative_path);

  const auto key = base::ToLowerASCII(base::WideToUTF8(relative_path->value()));

  if (const auto* payload = base::FindOrNull(GetIntegrityConfigCache(), key))
    return *payload;

  LOG(FATAL) << "Failed to find file integrity info for " << key;
}

}  // namespace asar
