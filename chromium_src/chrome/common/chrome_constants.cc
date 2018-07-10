// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_constants.h"

#define FPL FILE_PATH_LITERAL

namespace chrome {

#if defined(OS_MACOSX)
const base::FilePath::CharType kFrameworkName[] =
    FPL(ATOM_PRODUCT_NAME " Framework.framework");
#endif  // OS_MACOSX

// filenames
const base::FilePath::CharType kCacheDirname[] = FPL("Cache");
const base::FilePath::CharType kChannelIDFilename[] = FPL("Origin Bound Certs");
const base::FilePath::CharType kCookieFilename[] = FPL("Cookies");
const base::FilePath::CharType kCRLSetFilename[] =
    FPL("Certificate Revocation Lists");
const base::FilePath::CharType kCustomDictionaryFileName[] =
    FPL("Custom Dictionary.txt");
const base::FilePath::CharType kExtensionActivityLogFilename[] =
    FPL("Extension Activity");
const base::FilePath::CharType kExtensionsCookieFilename[] =
    FPL("Extension Cookies");
const base::FilePath::CharType kFirstRunSentinel[] = FPL("First Run");
const base::FilePath::CharType kGCMStoreDirname[] = FPL("GCM Store");
const base::FilePath::CharType kLocalStateFilename[] = FPL("Local State");
const base::FilePath::CharType kLocalStorePoolName[] = FPL("LocalStorePool");
const base::FilePath::CharType kMediaCacheDirname[] = FPL("Media Cache");
const base::FilePath::CharType kNetworkPersistentStateFilename[] =
    FPL("Network Persistent State");
const base::FilePath::CharType kOfflinePageArchviesDirname[] =
    FPL("Offline Pages/archives");
const base::FilePath::CharType kOfflinePageMetadataDirname[] =
    FPL("Offline Pages/metadata");
const base::FilePath::CharType kPreferencesFilename[] = FPL("Preferences");
const base::FilePath::CharType kProtectedPreferencesFilenameDeprecated[] =
    FPL("Protected Preferences");
const base::FilePath::CharType kReadmeFilename[] = FPL("README");
const base::FilePath::CharType kResetPromptMementoFilename[] =
    FPL("Reset Prompt Memento");
const base::FilePath::CharType kSafeBrowsingBaseFilename[] =
    FPL("Safe Browsing");
const base::FilePath::CharType kSecurePreferencesFilename[] =
    FPL("Secure Preferences");
const base::FilePath::CharType kServiceStateFileName[] = FPL("Service State");
const base::FilePath::CharType kSingletonCookieFilename[] =
    FPL("SingletonCookie");
const base::FilePath::CharType kSingletonLockFilename[] = FPL("SingletonLock");
const base::FilePath::CharType kSingletonSocketFilename[] =
    FPL("SingletonSocket");
const base::FilePath::CharType kSupervisedUserSettingsFilename[] =
    FPL("Managed Mode Settings");
const base::FilePath::CharType kThemePackFilename[] = FPL("Cached Theme.pak");
const base::FilePath::CharType kThemePackMaterialDesignFilename[] =
    FPL("Cached Theme Material Design.pak");
const base::FilePath::CharType kWebAppDirname[] = FPL("Web Applications");

// File name of the Pepper Flash plugin on different platforms.
const base::FilePath::CharType kPepperFlashPluginFilename[] =
#if defined(OS_MACOSX)
    FPL("PepperFlashPlayer.plugin");
#elif defined(OS_WIN)
    FPL("pepflashplayer.dll");
#else  // OS_LINUX, etc.
    FPL("libpepflashplayer.so");
#endif

}  // namespace chrome

#undef FPL
