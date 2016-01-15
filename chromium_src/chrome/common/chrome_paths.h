// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CHROME_PATHS_H__
#define CHROME_COMMON_CHROME_PATHS_H__

#include "build/build_config.h"

namespace base {
class FilePath;
}

// This file declares path keys for the chrome module.  These can be used with
// the PathService to access various special directories and files.

namespace chrome {

enum {
  PATH_START = 1000,

  DIR_APP = PATH_START,         // Directory where dlls and data reside.
  DIR_LOGS,                     // Directory where logs should be written.
  DIR_USER_DATA,                // Directory where user data can be written.
  DIR_CRASH_DUMPS,              // Directory where crash dumps are written.
#if defined(OS_WIN)
  DIR_WATCHER_DATA,             // Directory where the Chrome watcher stores
                                // data.
#endif
  DIR_RESOURCES,                // Directory containing separate file resources
                                // used by Chrome at runtime.
  DIR_INSPECTOR,                // Directory where web inspector is located.
  DIR_APP_DICTIONARIES,         // Directory where the global dictionaries are.
  DIR_USER_DOCUMENTS,           // Directory for a user's "My Documents".
  DIR_USER_MUSIC,               // Directory for a user's music.
  DIR_USER_PICTURES,            // Directory for a user's pictures.
  DIR_USER_VIDEOS,              // Directory for a user's videos.
  DIR_DEFAULT_DOWNLOADS_SAFE,   // Directory for a user's
                                // "My Documents/Downloads", (Windows) or
                                // "Downloads". (Linux)
  DIR_DEFAULT_DOWNLOADS,        // Directory for a user's downloads.
  DIR_INTERNAL_PLUGINS,         // Directory where internal plugins reside.
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  DIR_POLICY_FILES,             // Directory for system-wide read-only
                                // policy files that allow sys-admins
                                // to set policies for chrome. This directory
                                // contains subdirectories.
#endif
#if defined(OS_MACOSX) && !defined(OS_IOS)
  DIR_USER_APPLICATIONS,        // ~/Applications
  DIR_USER_LIBRARY,             // ~/Library
#endif
#if defined(OS_CHROMEOS) || (defined(OS_LINUX) && defined(CHROMIUM_BUILD)) || \
    (defined(OS_MACOSX) && !defined(OS_IOS))
  DIR_USER_EXTERNAL_EXTENSIONS,  // Directory for per-user external extensions
                                 // on Chrome Mac and Chromium Linux.
                                 // On Chrome OS, this path is used for OEM
                                 // customization. Getting this path does not
                                 // create it.
#endif

#if defined(OS_LINUX)
  DIR_STANDALONE_EXTERNAL_EXTENSIONS,  // Directory for 'per-extension'
                                       // definition manifest files that
                                       // describe extensions which are to be
                                       // installed when chrome is run.
#endif
  DIR_EXTERNAL_EXTENSIONS,      // Directory where installer places .crx files.

  DIR_DEFAULT_APPS,             // Directory where installer places .crx files
                                // to be installed when chrome is first run.
  DIR_PEPPER_FLASH_PLUGIN,      // Directory to the bundled Pepper Flash plugin,
                                // containing the plugin and the manifest.
  DIR_COMPONENT_UPDATED_PEPPER_FLASH_PLUGIN,  // Base directory of the Pepper
                                              // Flash plugins downloaded by the
                                              // component updater.
  FILE_RESOURCE_MODULE,         // Full path and filename of the module that
                                // contains embedded resources (version,
                                // strings, images, etc.).
  FILE_LOCAL_STATE,             // Path and filename to the file in which
                                // machine/installation-specific state is saved.
  FILE_RECORDED_SCRIPT,         // Full path to the script.log file that
                                // contains recorded browser events for
                                // playback.
  FILE_PEPPER_FLASH_PLUGIN,     // Full path to the bundled Pepper Flash plugin
                                // file.
  FILE_PEPPER_FLASH_SYSTEM_PLUGIN,  // Full path to the system version of the
                                    // Pepper Flash plugin, downloadable from
                                    // Adobe website. Querying this path might
                                    // succeed no matter the file exists or not.
  FILE_FLASH_SYSTEM_PLUGIN,     // Full path to the system version of NPAPI
                                // Flash plugin, downloadable from Adobe
                                // website. Querying this path might succeed no
                                // matter the file exists or not.
  FILE_NACL_PLUGIN,             // Full path to the internal NaCl plugin file.
  DIR_PNACL_BASE,               // Full path to the base dir for PNaCl.
  DIR_PNACL_COMPONENT,          // Full path to the latest PNaCl version
                                // (subdir of DIR_PNACL_BASE).
  DIR_COMPONENT_WIDEVINE_CDM,   // Directory that contains component-updated
                                // Widevine CDM files.
  FILE_WIDEVINE_CDM_ADAPTER,    // Full path to the Widevine CDM adapter file.
  FILE_RESOURCES_PACK,          // Full path to the .pak file containing
                                // binary data (e.g., html files and images
                                // used by internal pages).
  DIR_RESOURCES_EXTENSION,      // Full path to extension resources.
#if defined(OS_CHROMEOS)
  DIR_CHROMEOS_WALLPAPERS,      // Directory where downloaded chromeos
                                // wallpapers reside.
  DIR_CHROMEOS_WALLPAPER_THUMBNAILS,  // Directory where downloaded chromeos
                                      // wallpaper thumbnails reside.
  DIR_CHROMEOS_CUSTOM_WALLPAPERS,     // Directory where custom wallpapers
                                      // reside.
#endif
  DIR_SUPERVISED_USERS_DEFAULT_APPS,  // Directory where installer places .crx
                                      // files to be installed when managed user
                                      // session starts.
  DIR_SUPERVISED_USER_INSTALLED_WHITELISTS,  // Directory where sanitized
                                             // supervised user whitelists are
                                             // installed.
#if defined(OS_LINUX) || (defined(OS_MACOSX) && !defined(OS_IOS))
  DIR_NATIVE_MESSAGING,         // System directory where native messaging host
                                // manifest files are stored.
  DIR_USER_NATIVE_MESSAGING,    // Directory with Native Messaging Hosts
                                // installed per-user.
#endif
#if !defined(OS_ANDROID)
  DIR_GLOBAL_GCM_STORE,         // Directory where the global GCM instance
                                // stores its data.
#endif

  // Valid only in development environment; TODO(darin): move these
  DIR_GEN_TEST_DATA,            // Directory where generated test data resides.
  DIR_TEST_DATA,                // Directory where unit test data resides.
  DIR_TEST_TOOLS,               // Directory where unit test tools reside.
#if defined(OS_LINUX)
  FILE_COMPONENT_FLASH_HINT,    // A file in a known location that points to
                                // the component updated flash plugin.
#endif // defined(OS_LINUX)

  PATH_END
};

// Call once to register the provider for the path keys defined above.
void RegisterPathProvider();

// Get or set the invalid user data dir that was originally specified.
void SetInvalidSpecifiedUserDataDir(const base::FilePath& user_data_dir);
const base::FilePath& GetInvalidSpecifiedUserDataDir();

}  // namespace chrome

#endif  // CHROME_COMMON_CHROME_PATHS_H__
