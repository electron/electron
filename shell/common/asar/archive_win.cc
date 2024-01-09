// Copyright 2023 Slack Technologies, Inc.
// Contributors: Weiyun Dai (https://github.com/WeiyunD/), Andrew Lay
// (https://github.com/guohaolay) Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <algorithm>
#include <sstream>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/common/asar/asar_util.h"

using namespace std;
using namespace base;

#define INTEGRITY_CHECK_RESOURCE_TYPE L"Integrity"
#define INTEGRITY_CHECK_RESOURCE_ITEM L"ElectronAsar"

namespace asar {

/*
  Windows integrity payload cache
    - key: file rel path
    - value: integrity payload
  absl::nullopt: cache not initialized
  empty map: cache initialized but no data.
*/
static base::NoDestructor<
    absl::optional<unordered_map<string, IntegrityPayload>>>
    s_win_integrity_payloads_cache;

absl::optional<base::FilePath> Archive::RelativePath() const {
  // In case user has Windows long path enabled, the exe path should still no
  // larger than the asar path.
  size_t buf_len = max(path_.value().size(), (size_t)MAX_PATH);
  wchar_t* buf = new (std::nothrow) wchar_t[buf_len];
  if (!buf) {
    LOG(FATAL) << "Failed to alloc enough space to store exe file path";
    return absl::nullopt;
  }

  DWORD len = GetModuleFileName(NULL, buf, buf_len);
  wstring exe_path = wstring(buf, len);
  delete[] buf;

  if (!len) {
    LOG(FATAL) << "GetModuleFileName failed. Last error: " << GetLastError();
    return absl::nullopt;
  }

  base::FilePath exe_dir = base::FilePath(exe_path).DirName();
  base::FilePath relative_path;
  if (!exe_dir.AppendRelativePath(path_, &relative_path)) {
    return absl::nullopt;
  }

  return relative_path;
}

void LoadIntegrityConfigCache() {
  // Skip loading if cache is already loaded
  if (s_win_integrity_payloads_cache->has_value()) {
    return;
  }

  // Init cache
  *s_win_integrity_payloads_cache = unordered_map<string, IntegrityPayload>();

  // Load integrity config from exe resource
  HMODULE module_handle = ::GetModuleHandle(NULL);

  HRSRC resource = ::FindResource(module_handle, INTEGRITY_CHECK_RESOURCE_ITEM,
                                  INTEGRITY_CHECK_RESOURCE_TYPE);
  if (!resource) {
    LOG(FATAL) << "FindResource failed. Last error: " << GetLastError();
    return;
  }

  HGLOBAL rcData = ::LoadResource(module_handle, resource);
  if (!rcData) {
    LOG(FATAL) << "LoadResource failed. Last error: " << GetLastError();
    return;
  }

  auto* res_data = static_cast<const char*>(::LockResource(rcData));
  int res_size = SizeofResource(NULL, resource);

  if (!res_data || !res_size) {
    LOG(FATAL) << "Failed to integrity config from exe resource.";
    return;
  }

  // Parse integrity config payload
  auto integrity_config_payload = string(res_data, res_size);
  absl::optional<base::Value> root = base::JSONReader::Read(
      integrity_config_payload,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);

  if (!root.has_value()) {
    LOG(FATAL) << "Invalid integrity config: NOT a valid JSON.";
    return;
  }

  const base::Value::List* file_configs = root.value().GetIfList();
  if (!file_configs) {
    LOG(FATAL) << "Invalid integrity config: NOT a list.";
    return;
  }

  // Parse each individual file integrity config
  for (size_t i = 0; i < file_configs->size(); i++) {
    // Skip invalid file configs
    const Value::Dict* ele_dict = (*file_configs)[i].GetIfDict();
    if (!ele_dict) {
      LOG(WARNING) << "Skip config " << i << ": NOT a valid dict";
      continue;
    }

    const string* file = ele_dict->FindString("file");
    if (!file || file->empty()) {
      LOG(WARNING) << "Skip config " << i << ": Invalid file";
      continue;
    }

    const string* alg = ele_dict->FindString("alg");
    if (!alg || base::ToLowerASCII(*alg) != "sha256") {
      LOG(WARNING) << "Skip config " << i << ": Invalid alg";
      continue;
    }

    const string* value = ele_dict->FindString("value");
    if (!value || value->empty()) {
      LOG(WARNING) << "Skip config " << i << ": Invalid hash value";
      continue;
    }

    // Add valid file config into cache
    IntegrityPayload header_integrity;
    header_integrity.algorithm = HashAlgorithm::kSHA256;
    header_integrity.hash = *value;

    s_win_integrity_payloads_cache->value()[ToLowerASCII(*file)] =
        header_integrity;
  }
}

absl::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  absl::optional<base::FilePath> relative_path = RelativePath();
  // Callers should have already asserted this
  CHECK(relative_path.has_value());

  // Load integrity config from exe resource
  LoadIntegrityConfigCache();
  if (!s_win_integrity_payloads_cache->has_value()) {
    LOG(WARNING) << "Failed to integrity config from exe resource.";
    return absl::nullopt;
  }

  // Convert Window rel path to ASCII lower case
  string rel_path_utf8 = WideToASCII(relative_path.value().value().c_str());
  rel_path_utf8 = ToLowerASCII(rel_path_utf8);

  // Find file integrity config
  auto iter = s_win_integrity_payloads_cache->value().find(rel_path_utf8);
  if (iter == s_win_integrity_payloads_cache->value().end()) {
    LOG(FATAL) << "Failed to find file integrity info for " << rel_path_utf8;
    return absl::nullopt;
  }

  return iter->second;
}

}  // namespace asar
