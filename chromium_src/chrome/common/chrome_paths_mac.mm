// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/chrome_paths_internal.h"

#import <Foundation/Foundation.h>
#include <string.h>

#include <string>

#include "base/base_paths.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/scoped_nsautorelease_pool.h"
#include "base/memory/free_deleter.h"
#include "base/path_service.h"
#include "chrome/common/chrome_constants.h"

namespace {

#if !defined(OS_IOS)
const base::FilePath* g_override_versioned_directory = NULL;

// Return a retained (NOT autoreleased) NSBundle* as the internal
// implementation of chrome::OuterAppBundle(), which should be the only
// caller.
NSBundle* OuterAppBundleInternal() {
  base::mac::ScopedNSAutoreleasePool pool;

  if (!base::mac::AmIBundled()) {
    // If unbundled (as in a test), there's no app bundle.
    return nil;
  }

  if (!base::mac::IsBackgroundOnlyProcess()) {
    // Shortcut: in the browser process, just return the main app bundle.
    return [[NSBundle mainBundle] retain];
  }

  // From C.app/Contents/Versions/1.2.3.4, go up three steps to get to C.app.
  base::FilePath versioned_dir = chrome::GetVersionedDirectory();
  base::FilePath outer_app_dir = versioned_dir.DirName().DirName().DirName();
  const char* outer_app_dir_c = outer_app_dir.value().c_str();
  NSString* outer_app_dir_ns = [NSString stringWithUTF8String:outer_app_dir_c];

  return [[NSBundle bundleWithPath:outer_app_dir_ns] retain];
}
#endif  // !defined(OS_IOS)

char* ProductDirNameForBundle(NSBundle* chrome_bundle) {
  const char* product_dir_name = NULL;
#if !defined(OS_IOS)
  base::mac::ScopedNSAutoreleasePool pool;

  NSString* product_dir_name_ns =
      [chrome_bundle objectForInfoDictionaryKey:@"CrProductDirName"];
  product_dir_name = [product_dir_name_ns fileSystemRepresentation];
#else
  DCHECK(!chrome_bundle);
#endif

  if (!product_dir_name) {
#if defined(GOOGLE_CHROME_BUILD)
    product_dir_name = "Google/Chrome";
#else
    product_dir_name = "Chromium";
#endif
  }

  // Leaked, but the only caller initializes a static with this result, so it
  // only happens once, and that's OK.
  return strdup(product_dir_name);
}

// ProductDirName returns the name of the directory inside
// ~/Library/Application Support that should hold the product application
// data. This can be overridden by setting the CrProductDirName key in the
// outer browser .app's Info.plist. The default is "Google/Chrome" for
// officially-branded builds, and "Chromium" for unbranded builds. For the
// official canary channel, the Info.plist will have CrProductDirName set
// to "Google/Chrome Canary".
std::string ProductDirName() {
#if defined(OS_IOS)
  static const char* product_dir_name = ProductDirNameForBundle(nil);
#else
  // Use OuterAppBundle() to get the main app's bundle. This key needs to live
  // in the main app's bundle because it will be set differently on the canary
  // channel, and the autoupdate system dictates that there can be no
  // differences between channels within the versioned directory. This would
  // normally use base::mac::FrameworkBundle(), but that references the
  // framework bundle within the versioned directory. Ordinarily, the profile
  // should not be accessed from non-browser processes, but those processes do
  // attempt to get the profile directory, so direct them to look in the outer
  // browser .app's Info.plist for the CrProductDirName key.
  static const char* product_dir_name =
      ProductDirNameForBundle(chrome::OuterAppBundle());
#endif
  return std::string(product_dir_name);
}

bool GetDefaultUserDataDirectoryForProduct(const std::string& product_dir,
                                           base::FilePath* result) {
  bool success = false;
  if (result && PathService::Get(base::DIR_APP_DATA, result)) {
    *result = result->Append(product_dir);
    success = true;
  }
  return success;
}

}  // namespace

namespace chrome {

bool GetDefaultUserDataDirectory(base::FilePath* result) {
  return GetDefaultUserDataDirectoryForProduct(ProductDirName(), result);
}

bool GetUserDocumentsDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSDocumentDirectory, result);
}

void GetUserCacheDirectory(const base::FilePath& profile_dir,
                           base::FilePath* result) {
  // If the profile directory is under ~/Library/Application Support,
  // use a suitable cache directory under ~/Library/Caches.  For
  // example, a profile directory of ~/Library/Application
  // Support/Google/Chrome/MyProfileName would use the cache directory
  // ~/Library/Caches/Google/Chrome/MyProfileName.

  // Default value in cases where any of the following fails.
  *result = profile_dir;

  base::FilePath app_data_dir;
  if (!PathService::Get(base::DIR_APP_DATA, &app_data_dir))
    return;
  base::FilePath cache_dir;
  if (!PathService::Get(base::DIR_CACHE, &cache_dir))
    return;
  if (!app_data_dir.AppendRelativePath(profile_dir, &cache_dir))
    return;

  *result = cache_dir;
}

bool GetUserDownloadsDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSDownloadsDirectory, result);
}

bool GetUserMusicDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSMusicDirectory, result);
}

bool GetUserPicturesDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSPicturesDirectory, result);
}

bool GetUserVideosDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSMoviesDirectory, result);
}

#if !defined(OS_IOS)

base::FilePath GetVersionedDirectory() {
  if (g_override_versioned_directory)
    return *g_override_versioned_directory;

  // Start out with the path to the running executable.
  base::FilePath path;
  PathService::Get(base::FILE_EXE, &path);

  // One step up to MacOS, another to Contents.
  path = path.DirName().DirName();
  DCHECK_EQ(path.BaseName().value(), "Contents");

  if (base::mac::IsBackgroundOnlyProcess()) {
    // path identifies the helper .app's Contents directory in the browser
    // .app's versioned directory.  Go up two steps to get to the browser
    // .app's versioned directory.
    path = path.DirName().DirName();
  } else {
    // Go into the versioned directory.
    path = path.Append("Frameworks");
  }

  return path;
}

void SetOverrideVersionedDirectory(const base::FilePath* path) {
  if (path != g_override_versioned_directory) {
    delete g_override_versioned_directory;
    g_override_versioned_directory = path;
  }
}

base::FilePath GetFrameworkBundlePath() {
  // It's tempting to use +[NSBundle bundleWithIdentifier:], but it's really
  // slow (about 30ms on 10.5 and 10.6), despite Apple's documentation stating
  // that it may be more efficient than +bundleForClass:.  +bundleForClass:
  // itself takes 1-2ms.  Getting an NSBundle from a path, on the other hand,
  // essentially takes no time at all, at least when the bundle has already
  // been loaded as it will have been in this case.  The FilePath operations
  // needed to compute the framework's path are also effectively free, so that
  // is the approach that is used here.  NSBundle is also documented as being
  // not thread-safe, and thread safety may be a concern here.

  // The framework bundle is at a known path and name from the browser .app's
  // versioned directory.
  return GetVersionedDirectory().Append(kFrameworkName);
}

bool GetLocalLibraryDirectory(base::FilePath* result) {
  return base::mac::GetLocalDirectory(NSLibraryDirectory, result);
}

bool GetUserLibraryDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSLibraryDirectory, result);
}

bool GetUserApplicationsDirectory(base::FilePath* result) {
  return base::mac::GetUserDirectory(NSApplicationDirectory, result);
}

bool GetGlobalApplicationSupportDirectory(base::FilePath* result) {
  return base::mac::GetLocalDirectory(NSApplicationSupportDirectory, result);
}

NSBundle* OuterAppBundle() {
  // Cache this. Foundation leaks it anyway, and this should be the only call
  // to OuterAppBundleInternal().
  static NSBundle* bundle = OuterAppBundleInternal();
  return bundle;
}

bool GetUserDataDirectoryForBrowserBundle(NSBundle* bundle,
                                          base::FilePath* result) {
  std::unique_ptr<char, base::FreeDeleter>
      product_dir_name(ProductDirNameForBundle(bundle));
  return GetDefaultUserDataDirectoryForProduct(product_dir_name.get(), result);
}

#endif  // !defined(OS_IOS)

bool ProcessNeedsProfileDir(const std::string& process_type) {
  // For now we have no reason to forbid this on other MacOS as we don't
  // have the roaming profile troubles there.
  return true;
}

}  // namespace chrome
