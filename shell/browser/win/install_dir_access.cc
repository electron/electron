// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/win/install_dir_access.h"

#include <windows.h>

#include <optional>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win/access_token.h"
#include "base/win/security_descriptor.h"
#include "sandbox/win/src/restricted_token_utils.h"

namespace electron {

namespace {

// Asks the kernel whether the initial token that sandboxed children run with
// until they lower it (the token they open their data files with) may read
// |path|. The token is only used for the access check; nothing impersonates
// it.
bool SandboxTokenCanRead(const base::FilePath& path) {
  std::optional<base::win::AccessToken> token = sandbox::CreateRestrictedToken(
      sandbox::USER_RESTRICTED_SAME_ACCESS, sandbox::INTEGRITY_LEVEL_LOW,
      sandbox::TokenType::kImpersonation,
      /*lockdown_default_dacl=*/false, std::nullopt, std::nullopt);
  if (!token)
    return true;
  std::optional<base::win::SecurityDescriptor> descriptor =
      base::win::SecurityDescriptor::FromFile(
          path, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                    DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION);
  if (!descriptor)
    return true;
  std::optional<base::win::AccessCheckResult> result = descriptor->AccessCheck(
      *token, FILE_GENERIC_READ, base::win::SecurityObjectType::kFile);
  return !result || result->access_status;
}

}  // namespace

void CheckSandboxedProcessesCanReadInstallDir() {
  base::FilePath assets_dir;
  if (!base::PathService::Get(base::DIR_ASSETS, &assets_dir))
    return;
  LOG_IF(FATAL, !SandboxTokenCanRead(
                    assets_dir.Append(FILE_PATH_LITERAL("icudtl.dat"))))
      << "Sandboxed processes cannot read " << assets_dir.value()
      << ": its ACL has an entry for an AppContainer package SID but none "
         "for ALL APPLICATION PACKAGES, so Windows denies the sandbox token "
         "access. Grant it with: icacls \""
      << assets_dir.value() << "\" /grant *S-1-15-2-1:(OI)(CI)(RX)";
}

}  // namespace electron
