// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/platform_util.h"

#include <string>
#include <utility>

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <ServiceManagement/ServiceManagement.h>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_aedesc.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

namespace {

// This may be called from a global dispatch queue, the methods used here are
// thread safe, including LSGetApplicationForURL (> 10.2) and
// NSWorkspace#openURLs.
std::string OpenURL(NSURL* ns_url, bool activate) {
  CFURLRef ref = LSCopyDefaultApplicationURLForURL(
      base::mac::NSToCFCast(ns_url), kLSRolesAll, nullptr);

  // If no application could be found, NULL is returned and outError
  // (if not NULL) is populated with kLSApplicationNotFoundErr.
  if (ref == NULL)
    return "No application in the Launch Services database matches the input "
           "criteria.";

  NSUInteger launchOptions = NSWorkspaceLaunchDefault;
  if (!activate)
    launchOptions |= NSWorkspaceLaunchWithoutActivation;

  bool opened = [[NSWorkspace sharedWorkspace] openURLs:@[ ns_url ]
                                withAppBundleIdentifier:nil
                                                options:launchOptions
                         additionalEventParamDescriptor:nil
                                      launchIdentifiers:nil];
  if (!opened)
    return "Failed to open URL";

  return "";
}

NSString* GetLoginHelperBundleIdentifier() {
  return [[[NSBundle mainBundle] bundleIdentifier]
      stringByAppendingString:@".loginhelper"];
}

std::string OpenPathOnThread(const base::FilePath& full_path) {
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  NSURL* url = [NSURL fileURLWithPath:path_string];
  if (!url)
    return "Invalid path";

  const NSWorkspaceLaunchOptions launch_options =
      NSWorkspaceLaunchAsync | NSWorkspaceLaunchWithErrorPresentation;
  BOOL success = [[NSWorkspace sharedWorkspace] openURLs:@[ url ]
                                 withAppBundleIdentifier:nil
                                                 options:launch_options
                          additionalEventParamDescriptor:nil
                                       launchIdentifiers:NULL];

  return success ? "" : "Failed to open path";
}

}  // namespace

namespace platform_util {

void ShowItemInFolder(const base::FilePath& path) {
  // The API only takes absolute path.
  base::FilePath full_path =
      path.IsAbsolute() ? path : base::MakeAbsoluteFilePath(path);

  DCHECK([NSThread isMainThread]);
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string || ![[NSWorkspace sharedWorkspace] selectFile:path_string
                                        inFileViewerRootedAtPath:@""]) {
    LOG(WARNING) << "NSWorkspace failed to select file " << full_path.value();
  }
}

void OpenPath(const base::FilePath& full_path, OpenCallback callback) {
  std::move(callback).Run(OpenPathOnThread(full_path));
}

void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  OpenCallback callback) {
  DCHECK([NSThread isMainThread]);
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (!ns_url) {
    std::move(callback).Run("Invalid URL");
    return;
  }

  bool activate = options.activate;
  __block OpenCallback c = std::move(callback);
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                 ^{
                   __block std::string error = OpenURL(ns_url, activate);
                   dispatch_async(dispatch_get_main_queue(), ^{
                     std::move(c).Run(error);
                   });
                 });
}

bool MoveItemToTrash(const base::FilePath& full_path, bool delete_on_fail) {
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string) {
    LOG(WARNING) << "Invalid file path " << full_path.value();
    return false;
  }

  NSURL* url = [NSURL fileURLWithPath:path_string];
  NSError* err = nil;
  BOOL did_trash = [[NSFileManager defaultManager] trashItemAtURL:url
                                                 resultingItemURL:nil
                                                            error:&err];

  if (delete_on_fail) {
    // Some volumes may not support a Trash folder or it may be disabled
    // so these methods will report failure by returning NO or nil and
    // an NSError with NSFeatureUnsupportedError.
    // Handle this by deleting the item as a fallback.
    if (!did_trash && [err code] == NSFeatureUnsupportedError) {
      did_trash = [[NSFileManager defaultManager] removeItemAtURL:url
                                                            error:&err];
    }
  }

  if (!did_trash) {
    LOG(WARNING) << "NSWorkspace failed to move file " << full_path.value()
                 << " to trash: "
                 << base::SysNSStringToUTF8([err localizedDescription]);
  }

  return did_trash;
}

void Beep() {
  NSBeep();
}

bool GetLoginItemEnabled() {
  BOOL enabled = NO;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  // SMJobCopyDictionary does not work in sandbox (see rdar://13626319)
  CFArrayRef jobs = SMCopyAllJobDictionaries(kSMDomainUserLaunchd);
#pragma clang diagnostic pop
  NSArray* jobs_ = CFBridgingRelease(jobs);
  NSString* identifier = GetLoginHelperBundleIdentifier();
  if (jobs_ && [jobs_ count] > 0) {
    for (NSDictionary* job in jobs_) {
      if ([identifier isEqualToString:[job objectForKey:@"Label"]]) {
        enabled = [[job objectForKey:@"OnDemand"] boolValue];
        break;
      }
    }
  }
  return enabled;
}

bool SetLoginItemEnabled(bool enabled) {
  NSString* identifier = GetLoginHelperBundleIdentifier();
  return SMLoginItemSetEnabled((__bridge CFStringRef)identifier, enabled);
}

}  // namespace platform_util
