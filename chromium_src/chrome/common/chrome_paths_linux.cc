// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_paths_internal.h"

#include "base/base_paths.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"

namespace chrome {

using base::nix::GetXDGDirectory;
using base::nix::GetXDGUserDirectory;
using base::nix::kDotConfigDir;
using base::nix::kXdgConfigHomeEnvVar;

namespace {

const char kDownloadsDir[] = "Downloads";
const char kMusicDir[] = "Music";
const char kPicturesDir[] = "Pictures";
const char kVideosDir[] = "Videos";

// Generic function for GetUser{Music,Pictures,Video}Directory.
bool GetUserMediaDirectory(const std::string& xdg_name,
                           const std::string& fallback_name,
                           base::FilePath* result) {
#if defined(OS_CHROMEOS)
  // No local media directories on CrOS.
  return false;
#else
  *result = GetXDGUserDirectory(xdg_name.c_str(), fallback_name.c_str());

  base::FilePath home;
  PathService::Get(base::DIR_HOME, &home);
  if (*result != home) {
    base::FilePath desktop;
    if (!PathService::Get(base::DIR_USER_DESKTOP, &desktop))
      return false;
    if (*result != desktop) {
      return true;
    }
  }

  *result = home.Append(fallback_name);
  return true;
#endif
}

}  // namespace

// See http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
// for a spec on where config files go.  The net effect for most
// systems is we use ~/.config/chromium/ for Chromium and
// ~/.config/google-chrome/ for official builds.
// (This also helps us sidestep issues with other apps grabbing ~/.chromium .)
bool GetDefaultUserDataDirectory(base::FilePath* result) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  base::FilePath config_dir(GetXDGDirectory(env.get(),
                                            kXdgConfigHomeEnvVar,
                                            kDotConfigDir));
#if defined(GOOGLE_CHROME_BUILD)
  *result = config_dir.Append("google-chrome");
#else
  *result = config_dir.Append("chromium");
#endif
  return true;
}

void GetUserCacheDirectory(const base::FilePath& profile_dir,
                           base::FilePath* result) {
  // See http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
  // for a spec on where cache files go.  Our rule is:
  // - if the user-data-dir in the standard place,
  //     use same subdirectory of the cache directory.
  //     (this maps ~/.config/google-chrome to ~/.cache/google-chrome as well
  //      as the same thing for ~/.config/chromium)
  // - otherwise, use the profile dir directly.

  // Default value in cases where any of the following fails.
  *result = profile_dir;

  std::unique_ptr<base::Environment> env(base::Environment::Create());

  base::FilePath cache_dir;
  if (!PathService::Get(base::DIR_CACHE, &cache_dir))
    return;
  base::FilePath config_dir(GetXDGDirectory(env.get(),
                                            kXdgConfigHomeEnvVar,
                                            kDotConfigDir));

  if (!config_dir.AppendRelativePath(profile_dir, &cache_dir))
    return;

  *result = cache_dir;
}

bool GetUserDocumentsDirectory(base::FilePath* result) {
  *result = GetXDGUserDirectory("DOCUMENTS", "Documents");
  return true;
}

bool GetUserDownloadsDirectorySafe(base::FilePath* result) {
  base::FilePath home;
  PathService::Get(base::DIR_HOME, &home);
  *result = home.Append(kDownloadsDir);
  return true;
}

bool GetUserDownloadsDirectory(base::FilePath* result) {
  *result = GetXDGUserDirectory("DOWNLOAD", kDownloadsDir);
  return true;
}

// We respect the user's preferred pictures location, unless it is
// ~ or their desktop directory, in which case we default to ~/Music.
bool GetUserMusicDirectory(base::FilePath* result) {
  return GetUserMediaDirectory("MUSIC", kMusicDir, result);
}

// We respect the user's preferred pictures location, unless it is
// ~ or their desktop directory, in which case we default to ~/Pictures.
bool GetUserPicturesDirectory(base::FilePath* result) {
  return GetUserMediaDirectory("PICTURES", kPicturesDir, result);
}

// We respect the user's preferred pictures location, unless it is
// ~ or their desktop directory, in which case we default to ~/Videos.
bool GetUserVideosDirectory(base::FilePath* result) {
  return GetUserMediaDirectory("VIDEOS", kVideosDir, result);
}

bool ProcessNeedsProfileDir(const std::string& process_type) {
  // For now we have no reason to forbid this on Linux as we don't
  // have the roaming profile troubles there. Moreover the Linux breakpad needs
  // profile dir access in all process if enabled on Linux.
  return true;
}

}  // namespace chrome
