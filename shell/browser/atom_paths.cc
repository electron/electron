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

}  // namespace

// We can not use PathService with a provider !
// PathService caches the returned value of the first request
// If the path depends on another values that changes later
// PathService returns the cached value not the updated value

// This cannot be done as a static initializer sadly since Visual Studio will
// eliminate this object file if there is no direct entry point into it.
void AppPathService::Register() {
  // base::PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

bool AppPathService::Get(const std::string& name, base::FilePath* path) {
  return AppPathService::Get(GetPathConstant(name), path);
}

bool AppPathService::Get(int key, base::FilePath* path) {
  bool succeed = false;
  if (key >= 0)
    succeed = base::PathService::Get(key, path);
  if (!succeed)
    succeed = AppPathService::GetDefault(key, path);
  return succeed;
}

bool AppPathService::Override(const std::string& name,
                              const base::FilePath& path) {
  return AppPathService::Override(GetPathConstant(name), path);
}

bool AppPathService::Override(int key, const base::FilePath& path) {
  bool succeed = false;
  if (key >= 0)
    // Path must be  absolute and never created by default
    succeed =
        base::PathService::OverrideAndCreateIfNeeded(key, path, true, false);
  return succeed;
}

bool AppPathService::GetDefault(const std::string& name, base::FilePath* path) {
  return AppPathService::GetDefault(GetPathConstant(name), path);
}

bool AppPathService::GetDefault(int key, base::FilePath* path) {
  switch (key) {
    case DIR_APP_DATA:
#if defined(USE_X11)
      GetLinuxAppDataPath(path);
      return true;
#else
      return false;
#endif
    case DIR_USER_DATA:
      AppPathService::Get(DIR_APP_DATA, path);
      *path = path->Append(
          base::FilePath::FromUTF8Unsafe(Browser::Get()->GetName()));
      return true;
    case DIR_USER_CACHE:
      AppPathService::Get(DIR_CACHE, path);
      *path = path->Append(
          base::FilePath::FromUTF8Unsafe(Browser::Get()->GetName()));
      return true;
    case DIR_APP_LOGS:
#if defined(OS_MACOSX)
      GetMacAppLogsPath(path);
#else
      AppPathService::Get(DIR_USER_DATA, path);
      *path = path->Append(base::FilePath::FromUTF8Unsafe("logs"));
#endif
      return true;
    default:
      return false;
  }
}

}  // namespace electron
