// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/win/install_dir_access.h"

#include <windows.h>

#include <optional>

#include "base/base_paths.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/win/access_token.h"
#include "base/win/security_util.h"
#include "base/win/sid.h"
#include "sandbox/win/src/restricted_token_utils.h"

namespace electron {

namespace {

// Opens |path| for reading while impersonating the initial token that
// sandboxed children run with until they lower it, which is the token they
// open their data files with.
bool SandboxTokenCanRead(const base::FilePath& path) {
  std::optional<base::win::AccessToken> token = sandbox::CreateRestrictedToken(
      sandbox::USER_RESTRICTED_SAME_ACCESS, sandbox::INTEGRITY_LEVEL_LOW,
      sandbox::TokenType::kImpersonation,
      /*lockdown_default_dacl=*/false, std::nullopt, std::nullopt);
  if (!token)
    return true;
  if (!::SetThreadToken(nullptr, token->get()))
    return true;
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  const bool denied =
      !file.IsValid() &&
      file.error_details() == base::File::FILE_ERROR_ACCESS_DENIED;
  ::RevertToSelf();
  return !denied;
}

}  // namespace

void EnsureSandboxedProcessesCanReadInstallDir() {
  base::FilePath assets_dir;
  if (!base::PathService::Get(base::DIR_ASSETS, &assets_dir))
    return;
  const base::FilePath probe =
      assets_dir.Append(FILE_PATH_LITERAL("icudtl.dat"));
  if (SandboxTokenCanRead(probe))
    return;

  LOG(WARNING) << "Sandboxed processes cannot read " << assets_dir.value()
               << " because its ACL has AppContainer entries without one for "
                  "ALL APPLICATION PACKAGES; granting read access.";
  const bool granted = base::win::GrantAccessToPath(
      assets_dir,
      base::win::Sid::FromKnownSidVector(
          {base::win::WellKnownSid::kAllApplicationPackages}),
      FILE_GENERIC_READ | FILE_GENERIC_EXECUTE,
      CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE, /*recursive=*/true);
  if (!granted || !SandboxTokenCanRead(probe)) {
    LOG(ERROR) << "Could not grant sandboxed processes read access to "
               << assets_dir.value()
               << ". GPU and renderer processes will fail to start; run "
                  "`icacls \"<dir>\" /grant *S-1-15-2-1:(OI)(CI)(RX)` on it.";
  }
}

}  // namespace electron
