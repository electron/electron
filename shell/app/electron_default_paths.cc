// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/app/electron_default_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "shell/browser/browser.h"

#if defined(OS_MAC)
#include "shell/app/electron_default_paths_mac.h"
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

bool GetDefaultCrashDumpsPath(base::FilePath* path) {
  base::FilePath cur;
  if (!base::PathService::Get(DIR_USER_DATA, &cur))
    return false;
#if defined(OS_MAC) || defined(OS_WIN)
  cur = cur.Append(FILE_PATH_LITERAL("Crashpad"));
#else
  cur = cur.Append(FILE_PATH_LITERAL("Crash Reports"));
#endif
  // TODO(bauerb): http://crbug.com/259796
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  if (!base::PathExists(cur) && !base::CreateDirectory(cur))
    return false;
  *path = cur;
  return true;
}

}  // namespace

bool ElectronDefaultPaths::GetDefault(int key, base::FilePath* path) {
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
#if defined(OS_MAC)
      GetMacAppLogsPath(path);
#else
      base::PathService::Get(DIR_USER_DATA, path);
      *path = path->Append(base::FilePath::FromUTF8Unsafe("logs"));
#endif
      return true;
    case DIR_CRASH_DUMPS:
      return GetDefaultCrashDumpsPath(path);
  }
  return false;
}

}  // namespace electron
