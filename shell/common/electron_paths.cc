// Copyright (c) 2026 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/electron_paths.h"

#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "shell/common/application_info.h"
#include "shell/common/platform_util.h"
#include "shell/common/thread_restrictions.h"

#if BUILDFLAG(IS_LINUX)
#include "base/nix/xdg_util.h"
#endif

namespace electron {

namespace {

bool ElectronPathProvider(int key, base::FilePath* result) {
  bool create_dir = false;
  base::FilePath cur;
  switch (key) {
    case chrome::DIR_USER_DATA:
      if (!base::PathService::Get(DIR_APP_DATA, &cur))
        return false;
      cur = cur.Append(base::FilePath::FromUTF8Unsafe(
          GetPossiblyOverriddenApplicationName()));
      create_dir = true;
      break;
    case DIR_CRASH_DUMPS:
      if (!base::PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Crashpad"));
      create_dir = true;
      break;
    case chrome::DIR_APP_DICTIONARIES:
      // TODO(nornagon): can we just default to using Chrome's logic here?
      if (!base::PathService::Get(DIR_SESSION_DATA, &cur))
        return false;
      cur = cur.Append(base::FilePath::FromUTF8Unsafe("Dictionaries"));
      create_dir = true;
      break;
    case DIR_SESSION_DATA:
      // By default and for backward, equivalent to DIR_USER_DATA.
      return base::PathService::Get(chrome::DIR_USER_DATA, result);
    case DIR_USER_CACHE: {
#if BUILDFLAG(IS_POSIX)
      int parent_key = base::DIR_CACHE;
#else
      // On Windows, there's no OS-level centralized location for caches, so
      // store the cache in the app data directory.
      int parent_key = base::DIR_ROAMING_APP_DATA;
#endif
      if (!base::PathService::Get(parent_key, &cur))
        return false;
      cur = cur.Append(base::FilePath::FromUTF8Unsafe(
          GetPossiblyOverriddenApplicationName()));
      create_dir = true;
      break;
    }
#if BUILDFLAG(IS_LINUX)
    case DIR_APP_DATA: {
      auto env = base::Environment::Create();
      cur = base::nix::GetXDGDirectory(
          env.get(), base::nix::kXdgConfigHomeEnvVar, base::nix::kDotConfigDir);
      break;
    }
#endif
#if BUILDFLAG(IS_WIN)
    case DIR_RECENT:
      if (!platform_util::GetFolderPath(DIR_RECENT, &cur))
        return false;
      create_dir = true;
      break;
#endif
    case DIR_APP_LOGS:
#if BUILDFLAG(IS_MAC)
      if (!base::PathService::Get(base::DIR_HOME, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Library"));
      cur = cur.Append(FILE_PATH_LITERAL("Logs"));
      cur = cur.Append(base::FilePath::FromUTF8Unsafe(
          GetPossiblyOverriddenApplicationName()));
#else
      if (!base::PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      cur = cur.Append(base::FilePath::FromUTF8Unsafe("logs"));
#endif
      create_dir = true;
      break;
    default:
      return false;
  }

  // TODO(bauerb): http://crbug.com/259796
  ScopedAllowBlockingForElectron allow_blocking;
  if (create_dir && !base::PathExists(cur) && !base::CreateDirectory(cur)) {
    return false;
  }

  *result = cur;

  return true;
}

}  // namespace

void RegisterPathProvider() {
  base::PathService::RegisterProvider(ElectronPathProvider, PATH_START,
                                      PATH_END);
}

}  // namespace electron
