#include "shell/common/asar/archive.h"

#include <algorithm>
#include <sstream>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "shell/common/asar/asar_util.h"

using namespace std;

#define INTEGRITY_CHECK_RESOURCE_TYPE L"Integrity"
#define INTEGRITY_CHECK_RESOURCE_ITEM L"ElectronAsar"

namespace asar {

HRSRC FindElectronAsarIntegrityResource(HMODULE handle) {
  HRSRC resource = ::FindResource(handle, INTEGRITY_CHECK_RESOURCE_ITEM,
                                  INTEGRITY_CHECK_RESOURCE_TYPE);
  if (!resource) {
    LOG(WARNING) << " " << __func__
                 << "(): FindResource failed. Last error: " << GetLastError();
    return NULL;
  }

  return resource;
}

/*
  Since the Windows asar header integrity config is part of the exe resource not
  a file on disk, returning a blank file path object when the config is present.
*/
absl::optional<base::FilePath> Archive::RelativePath() const {
  if (FindElectronAsarIntegrityResource(::GetModuleHandle(NULL))) {
    return base::FilePath();
  } else {
    return absl::nullopt;
  }
}

absl::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  HMODULE handle = ::GetModuleHandle(NULL);
  HRSRC resource = FindElectronAsarIntegrityResource(handle);

  HGLOBAL rcData = ::LoadResource(handle, resource);
  if (!rcData) {
    LOG(WARNING) << " " << __func__
                 << "(): LoadResource failed. Last error: " << GetLastError();
    return absl::nullopt;
  }

  auto* res_data = static_cast<const char*>(::LockResource(rcData));
  int res_size = SizeofResource(NULL, resource);

  if (!res_data || !res_size) {
    LOG(WARNING) << " " << __func__
                 << "(): Failed to integrity config from exe resource.";
    return absl::nullopt;
  }

  istringstream res_stream(string(res_data, res_size));
  string filepath, hash_alg, hash_value;
  if (!getline(res_stream, filepath) || !getline(res_stream, hash_alg) ||
      !getline(res_stream, hash_value)) {
    LOG(WARNING) << " " << __func__
                 << "(): Failed to read 3 lines from integrity config (invalid "
                    "config format).";
    return absl::nullopt;
  }

  auto trimLineEndingCR = [](string& line) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
  };

  trimLineEndingCR(filepath);
  trimLineEndingCR(hash_alg);
  trimLineEndingCR(hash_value);

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
