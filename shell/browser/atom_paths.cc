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

// Return the path constant from string.
int GetPathConstant(const std::string& name) {
  if (name == "appData")
    return DIR_APP_DATA;
  else if (name == "userData")
    return DIR_USER_DATA;
  else if (name == "cache")
    return DIR_CACHE;
  else if (name == "userCache")
    return DIR_USER_CACHE;
  else if (name == "logs")
    return DIR_APP_LOGS;
  else if (name == "home")
    return base::DIR_HOME;
  else if (name == "temp")
    return base::DIR_TEMP;
  else if (name == "userDesktop" || name == "desktop")
    return base::DIR_USER_DESKTOP;
  else if (name == "exe")
    return base::FILE_EXE;
  else if (name == "module")
    return base::FILE_MODULE;
  else if (name == "documents")
    return chrome::DIR_USER_DOCUMENTS;
  else if (name == "downloads")
    return chrome::DIR_DEFAULT_DOWNLOADS;
  else if (name == "music")
    return chrome::DIR_USER_MUSIC;
  else if (name == "pictures")
    return chrome::DIR_USER_PICTURES;
  else if (name == "videos")
    return chrome::DIR_USER_VIDEOS;
  else if (name == "pepperFlashSystemPlugin")
    return chrome::FILE_PEPPER_FLASH_SYSTEM_PLUGIN;
  else
    return -1;
}

#if defined(USE_X11)
void GetLinuxAppDataPath(base::FilePath* path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  *path = base::nix::GetXDGDirectory(env.get(), base::nix::kXdgConfigHomeEnvVar,
                                     base::nix::kDotConfigDir);
}
#endif

bool PathProvider(int key, base::FilePath* path) {
  switch (key) {
    case DIR_APP_DATA:
#if defined(USE_X11)
      GetLinuxAppDataPath(path);
      return true;
#else
      // Should be never reached as beyond PATH_END
      // return base::PathService::Get(base::DIR_APP_DATA, path);
      return false;
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
    default:
      return false;
  }
}

}  // namespace

// This cannot be done as a static initializer sadly since Visual Studio will
// eliminate this object file if there is no direct entry point into it.
void AppPathProvider::Register() {
  base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

bool AppPathProvider::GetPath(const std::string& name, base::FilePath* path) {
  bool succeed = false;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed = base::PathService::Get(key, path);
  return succeed;
}

bool AppPathProvider::SetPath(const std::string& name,
                              const base::FilePath& path) {
  bool succeed = false;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed =
        base::PathService::OverrideAndCreateIfNeeded(key, path, true, false);
  return succeed;
}

bool AppPathProvider::GetDefaultPath(const std::string& name,
                                     base::FilePath* path) {
  return PathProvider(GetPathConstant(name), path);
}

bool AppPathProvider::GetDefaultPath(int key, base::FilePath* path) {
  return PathProvider(key, path);
}

}  // namespace electron
