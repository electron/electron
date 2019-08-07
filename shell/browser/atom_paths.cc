// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_paths.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"

namespace electron {

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

bool AppPathProvider::GetPath(const std::string& name, base::FilePath& path) {
  bool succeed = false;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed = base::PathService::Get(key, &path);
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

boolean AppPathProvider::GetProviderFunc(int key, FilePath* path) {
  return false;
}

}  // namespace electron