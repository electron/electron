// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "shell/browser/browser.h"

#if defined(OS_MACOSX)
#include "shell/browser/atom_paths_mac.h"
#endif

#if defined(USE_X11)
#include <memory>
#include "base/environment.h"
#include "base/nix/xdg_util.h"
#endif

namespace electron {

namespace {

#if defined(USE_X11)
void GetLinuxAppDataPath(base::FilePath* path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  *path = base::nix::GetXDGDirectory(env.get(), base::nix::kXdgConfigHomeEnvVar,
                                     base::nix::kDotConfigDir);
}
#endif

}  // namespace

// We can not use PathService with a provider !
// PathService caches the returned value of the first request
// If the path depends on another values that changes later
// PathService returns the cached value not the updated value

// This cannot be done as a static initializer sadly since Visual Studio will
// eliminate this object file if there is no direct entry point into it.
void AtomPaths::Register() {
  base::PathService::RegisterProvider(AtomPaths::GetDefault, PATH_START,
                                      PATH_END);
}

bool AtomPaths::GetDefault(int key, base::FilePath* path) {
  switch (key) {
#if defined(USE_X11)
    case DIR_APP_DATA:
      GetLinuxAppDataPath(path);
      return true;
#endif
    case DIR_USER_DATA:
      base::PathService::Get(DIR_APP_DATA, path);
      *path = path->Append(
          base::FilePath::FromUTF8Unsafe(Browser::Get()->GetName()));
      return true;
    case DIR_USER_CACHE:
      base::PathService::Get(DIR_CACHE, path);
      *path = path->Append(
          base::FilePath::FromUTF8Unsafe(Browser::Get()->GetName()));
      return true;
    case DIR_APP_LOGS:
#if defined(OS_MACOSX)
      GetMacAppLogsPath(path);
#else
      base::PathService::Get(DIR_USER_DATA, path);
      *path = path->Append(base::FilePath::FromUTF8Unsafe("logs"));
#endif
      return true;
  }
  return false;
}

}  // namespace electron
