// Copyright 2023 Slack Technologies, Inc.
// Contributors: Weiyun Dai (https://github.com/WeiyunD/), Andrew Lay
// (https://github.com/guohaolay) Use of this source code is governed by the MIT
// license that can be found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <algorithm>
#include <sstream>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/string_util_win.h"
#include "shell/common/asar/asar_util.h"

using namespace std;

#define INTEGRITY_CHECK_RESOURCE_TYPE L"Integrity"
#define INTEGRITY_CHECK_RESOURCE_ITEM L"ElectronAsar"

namespace asar {

absl::optional<base::FilePath> Archive::RelativePath() const {
  // In case user has Windows long path enabled, the exe path should still no
  // larger than the asar path.
  size_t buf_len = max(path_.value().size(), (size_t)MAX_PATH);
  wchar_t* buf = new (std::nothrow) wchar_t[buf_len];
  if (!buf) {
    LOG(WARNING) << " " << __func__
                 << "(): Failed to alloc enough space to store exe file path";
    return absl::nullopt;
  }

  DWORD len = GetModuleFileName(NULL, buf, buf_len);
  wstring exe_path = wstring(buf, len);
  delete[] buf;

  if (!len) {
    LOG(WARNING) << " " << __func__
                 << "(): GetModuleFileName failed. Last error: "
                 << GetLastError();
    return absl::nullopt;
  }

  base::FilePath exe_dir = base::FilePath(exe_path).DirName();
  base::FilePath relative_path;
  if (!exe_dir.AppendRelativePath(path_, &relative_path)) {
    return absl::nullopt;
  }

  return relative_path;
}

string GetIntegrityConfig() {
  // Load integrity config from exe resource
  HMODULE module_handle = ::GetModuleHandle(NULL);

  HRSRC resource = ::FindResource(module_handle, INTEGRITY_CHECK_RESOURCE_ITEM,
                                  INTEGRITY_CHECK_RESOURCE_TYPE);
  if (!resource) {
    LOG(WARNING) << " " << __func__
                 << "(): FindResource failed. Last error: " << GetLastError();
    return "";
  }

  HGLOBAL rcData = ::LoadResource(module_handle, resource);
  if (!rcData) {
    LOG(WARNING) << " " << __func__
                 << "(): LoadResource failed. Last error: " << GetLastError();
    return "";
  }

  auto* res_data = static_cast<const char*>(::LockResource(rcData));
  int res_size = SizeofResource(NULL, resource);

  if (!res_data || !res_size) {
    LOG(WARNING) << " " << __func__
                 << "(): Failed to integrity config from exe resource.";
    return "";
  }

  return string(res_data, res_size);
}

absl::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  absl::optional<base::FilePath> relative_path = RelativePath();
  // Callers should have already asserted this
  CHECK(relative_path.has_value());

  // Load integrity config from exe resource
  string integrity_config = GetIntegrityConfig();
  if (integrity_config.empty()) {
    LOG(WARNING) << " " << __func__
                 << "(): Failed to integrity config from exe resource.";
    return absl::nullopt;
  }

  // Parse integrity config
  istringstream integrity_config_stream(integrity_config);
  string filepath, hash_alg, hash_value;
  while (!base::EqualsCaseInsensitiveASCII(filepath,
                                           relative_path.value().value())) {
    if (!getline(integrity_config_stream, filepath) ||
        !getline(integrity_config_stream, hash_alg) ||
        !getline(integrity_config_stream, hash_value)) {
      LOG(WARNING)
          << " " << __func__
          << "(): Failed to read 3 lines from integrity config (invalid "
             "config format).";
      return absl::nullopt;
    }

    base::TrimString(filepath, "\r", &filepath);  // Trim trailing \r
    base::TrimString(
        hash_alg, "\r\t ",
        &hash_alg);  // Trim leading tab or whitespace and trailing \r
    base::TrimString(
        hash_value, "\r\t ",
        &hash_value);  // Trim leading tab or whitespace and trailing \r
  }

  if (base::ToLowerASCII(hash_alg) != "sha256") {
    LOG(WARNING)
        << " " << __func__
        << "(): Unrecognized hash algorithm (only SHA256 is supported).";
    return absl::nullopt;
  }

  if (hash_value.empty()) {
    LOG(WARNING) << " " << __func__ << "(): Invalid (blank) hash value.";
    return absl::nullopt;
  }

  IntegrityPayload header_integrity;
  header_integrity.algorithm = HashAlgorithm::kSHA256;
  header_integrity.hash = hash_value;
  return header_integrity;
}

}  // namespace asar
