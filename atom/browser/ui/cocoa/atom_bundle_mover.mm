// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/atom_bundle_mover.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <dlfcn.h>
#import <sys/mount.h>
#import <sys/param.h>

#import "atom/browser/browser.h"

namespace atom {

namespace ui {

namespace cocoa {

bool AtomBundleMover::Move(mate::Arguments* args) {
  // Path of the current bundle
  NSString* bundlePath = [[NSBundle mainBundle] bundlePath];

  // Skip if the application is already in the Applications folder
  if (IsInApplicationsFolder(bundlePath))
    return true;

  NSFileManager* fileManager = [NSFileManager defaultManager];

  NSString* diskImageDevice = ContainingDiskImageDevice(bundlePath);

  NSString* applicationsDirectory = [[NSSearchPathForDirectoriesInDomains(
      NSApplicationDirectory, NSLocalDomainMask, true) lastObject]
      stringByResolvingSymlinksInPath];
  NSString* bundleName = [bundlePath lastPathComponent];
  NSString* destinationPath =
      [applicationsDirectory stringByAppendingPathComponent:bundleName];

  // Check if we can write to the applications directory
  // and then make sure that if the app already exists we can overwrite it
  bool needAuthorization =
      ![fileManager isWritableFileAtPath:applicationsDirectory] |
      ([fileManager fileExistsAtPath:destinationPath] &&
       ![fileManager isWritableFileAtPath:destinationPath]);

  // Activate app -- work-around for focus issues related to "scary file from
  // internet" OS dialog.
  if (![NSApp isActive]) {
    [NSApp activateIgnoringOtherApps:true];
  }

  // Move to applications folder
  if (needAuthorization) {
    bool authorizationCanceled;

    if (!AuthorizedInstall(bundlePath, destinationPath,
                           &authorizationCanceled)) {
      if (authorizationCanceled) {
        // User rejected the authorization request
        args->ThrowError("User rejected the authorization request");
        return false;
      } else {
        args->ThrowError(
            "Failed to copy to applications directory even with authorization");
        return false;
      }
    }
  } else {
    // If a copy already exists in the Applications folder, put it in the Trash
    if ([fileManager fileExistsAtPath:destinationPath]) {
      // But first, make sure that it's not running
      if (IsApplicationAtPathRunning(destinationPath)) {
        // Give the running app focus and terminate myself
        [[NSTask
            launchedTaskWithLaunchPath:@"/usr/bin/open"
                             arguments:[NSArray
                                           arrayWithObject:destinationPath]]
            waitUntilExit];
        atom::Browser::Get()->Quit();
        return true;
      } else {
        if (!Trash([applicationsDirectory
                stringByAppendingPathComponent:bundleName])) {
          args->ThrowError("Failed to delete existing application");
          return false;
        }
      }
    }

    if (!CopyBundle(bundlePath, destinationPath)) {
      args->ThrowError(
          "Failed to copy current bundle to the applications folder");
      return false;
    }
  }

  // Trash the original app. It's okay if this fails.
  // NOTE: This final delete does not work if the source bundle is in a network
  // mounted volume.
  //       Calling rm or file manager's delete method doesn't work either. It's
  //       unlikely to happen but it'd be great if someone could fix this.
  if (diskImageDevice == nil && !DeleteOrTrash(bundlePath)) {
    // Could not delete original but we just don't care
  }

  // Relaunch.
  Relaunch(destinationPath);

  // Launched from within a disk image? -- unmount (if no files are open after 5
  // seconds, otherwise leave it mounted).
  if (diskImageDevice) {
    NSString* script = [NSString
        stringWithFormat:@"(/bin/sleep 5 && /usr/bin/hdiutil detach %@) &",
                         ShellQuotedString(diskImageDevice)];
    [NSTask launchedTaskWithLaunchPath:@"/bin/sh"
                             arguments:[NSArray arrayWithObjects:@"-c", script,
                                                                 nil]];
  }

  atom::Browser::Get()->Quit();

  return true;
}

bool AtomBundleMover::IsCurrentAppInApplicationsFolder() {
  return IsInApplicationsFolder([[NSBundle mainBundle] bundlePath]);
}

bool AtomBundleMover::IsInApplicationsFolder(NSString* bundlePath) {
  // Check all the normal Application directories
  NSArray* applicationDirs = NSSearchPathForDirectoriesInDomains(
      NSApplicationDirectory, NSAllDomainsMask, true);
  for (NSString* appDir in applicationDirs) {
    if ([bundlePath hasPrefix:appDir])
      return true;
  }

  // Also, handle the case that the user has some other Application directory
  // (perhaps on a separate data partition).
  if ([[bundlePath pathComponents] containsObject:@"Applications"])
    return true;

  return false;
}

NSString* AtomBundleMover::ContainingDiskImageDevice(NSString* bundlePath) {
  NSString* containingPath = [bundlePath stringByDeletingLastPathComponent];

  struct statfs fs;
  if (statfs([containingPath fileSystemRepresentation], &fs) ||
      (fs.f_flags & MNT_ROOTFS))
    return nil;

  NSString* device = [[NSFileManager defaultManager]
      stringWithFileSystemRepresentation:fs.f_mntfromname
                                  length:strlen(fs.f_mntfromname)];

  NSTask* hdiutil = [[[NSTask alloc] init] autorelease];
  [hdiutil setLaunchPath:@"/usr/bin/hdiutil"];
  [hdiutil setArguments:[NSArray arrayWithObjects:@"info", @"-plist", nil]];
  [hdiutil setStandardOutput:[NSPipe pipe]];
  [hdiutil launch];
  [hdiutil waitUntilExit];

  NSData* data =
      [[[hdiutil standardOutput] fileHandleForReading] readDataToEndOfFile];

  NSDictionary* info =
      [NSPropertyListSerialization propertyListWithData:data
                                                options:NSPropertyListImmutable
                                                 format:NULL
                                                  error:NULL];

  if (![info isKindOfClass:[NSDictionary class]])
    return nil;

  NSArray* images = (NSArray*)[info objectForKey:@"images"];
  if (![images isKindOfClass:[NSArray class]])
    return nil;

  for (NSDictionary* image in images) {
    if (![image isKindOfClass:[NSDictionary class]])
      return nil;

    id systemEntities = [image objectForKey:@"system-entities"];
    if (![systemEntities isKindOfClass:[NSArray class]])
      return nil;

    for (NSDictionary* systemEntity in systemEntities) {
      if (![systemEntity isKindOfClass:[NSDictionary class]])
        return nil;

      NSString* devEntry = [systemEntity objectForKey:@"dev-entry"];
      if (![devEntry isKindOfClass:[NSString class]])
        return nil;

      if ([devEntry isEqualToString:device])
        return device;
    }
  }

  return nil;
}

bool AtomBundleMover::AuthorizedInstall(NSString* srcPath,
                                        NSString* dstPath,
                                        bool* canceled) {
  if (canceled)
    *canceled = false;

  // Make sure that the destination path is an app bundle. We're essentially
  // running 'sudo rm -rf' so we really don't want to screw this up.
  if (![[dstPath pathExtension] isEqualToString:@"app"])
    return false;

  // Do some more checks
  if ([[dstPath stringByTrimmingCharactersInSet:[NSCharacterSet
                                                    whitespaceCharacterSet]]
          length] == 0)
    return false;
  if ([[srcPath stringByTrimmingCharactersInSet:[NSCharacterSet
                                                    whitespaceCharacterSet]]
          length] == 0)
    return false;

  int pid, status;
  AuthorizationRef myAuthorizationRef;

  // Get the authorization
  OSStatus err =
      AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,
                          kAuthorizationFlagDefaults, &myAuthorizationRef);
  if (err != errAuthorizationSuccess)
    return false;

  AuthorizationItem myItems = {kAuthorizationRightExecute, 0, NULL, 0};
  AuthorizationRights myRights = {1, &myItems};
  AuthorizationFlags myFlags = (AuthorizationFlags)(
      kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights |
      kAuthorizationFlagPreAuthorize);

  err = AuthorizationCopyRights(myAuthorizationRef, &myRights, NULL, myFlags,
                                NULL);
  if (err != errAuthorizationSuccess) {
    if (err == errAuthorizationCanceled && canceled)
      *canceled = true;
    goto fail;
  }

  static OSStatus (*security_AuthorizationExecuteWithPrivileges)(
      AuthorizationRef authorization, const char* pathToTool,
      AuthorizationFlags options, char* const* arguments,
      FILE** communicationsPipe) = NULL;
  if (!security_AuthorizationExecuteWithPrivileges) {
    // On 10.7, AuthorizationExecuteWithPrivileges is deprecated. We want to
    // still use it since there's no good alternative (without requiring code
    // signing). We'll look up the function through dyld and fail if it is no
    // longer accessible. If Apple removes the function entirely this will fail
    // gracefully. If they keep the function and throw some sort of exception,
    // this won't fail gracefully, but that's a risk we'll have to take for now.
    security_AuthorizationExecuteWithPrivileges = (OSStatus(*)(
        AuthorizationRef, const char*, AuthorizationFlags, char* const*,
        FILE**))dlsym(RTLD_DEFAULT, "AuthorizationExecuteWithPrivileges");
  }
  if (!security_AuthorizationExecuteWithPrivileges)
    goto fail;

  // Delete the destination
  {
    char rf[] = "-rf";
    char* args[] = {rf, (char*)[dstPath fileSystemRepresentation], NULL};
    err = security_AuthorizationExecuteWithPrivileges(
        myAuthorizationRef, "/bin/rm", kAuthorizationFlagDefaults, args, NULL);
    if (err != errAuthorizationSuccess)
      goto fail;

    // Wait until it's done
    pid = wait(&status);
    if (pid == -1 || !WIFEXITED(status))
      goto fail;  // We don't care about exit status as the destination most
                  // likely does not exist
  }

  // Copy
  {
    char pR[] = "-pR";
    char* args[] = {pR, (char*)[srcPath fileSystemRepresentation],
                    (char*)[dstPath fileSystemRepresentation], NULL};
    err = security_AuthorizationExecuteWithPrivileges(
        myAuthorizationRef, "/bin/cp", kAuthorizationFlagDefaults, args, NULL);
    if (err != errAuthorizationSuccess)
      goto fail;

    // Wait until it's done
    pid = wait(&status);
    if (pid == -1 || !WIFEXITED(status) || WEXITSTATUS(status))
      goto fail;
  }

  AuthorizationFree(myAuthorizationRef, kAuthorizationFlagDefaults);
  return true;

fail:
  AuthorizationFree(myAuthorizationRef, kAuthorizationFlagDefaults);
  return false;
}

bool AtomBundleMover::CopyBundle(NSString* srcPath, NSString* dstPath) {
  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSError* error = nil;

  if ([fileManager copyItemAtPath:srcPath toPath:dstPath error:&error]) {
    return true;
  } else {
    return false;
  }
}

NSString* AtomBundleMover::ShellQuotedString(NSString* string) {
  return [NSString
      stringWithFormat:@"'%@'",
                       [string stringByReplacingOccurrencesOfString:@"'"
                                                         withString:@"'\\''"]];
}

void AtomBundleMover::Relaunch(NSString* destinationPath) {
  // The shell script waits until the original app process terminates.
  // This is done so that the relaunched app opens as the front-most app.
  int pid = [[NSProcessInfo processInfo] processIdentifier];

  // Command run just before running open /final/path
  NSString* preOpenCmd = @"";

  NSString* quotedDestinationPath = ShellQuotedString(destinationPath);

  // Before we launch the new app, clear xattr:com.apple.quarantine to avoid
  // duplicate "scary file from the internet" dialog.
  preOpenCmd = [NSString
      stringWithFormat:@"/usr/bin/xattr -d -r com.apple.quarantine %@",
                       quotedDestinationPath];

  NSString* script =
      [NSString stringWithFormat:
                    @"(while /bin/kill -0 %d >&/dev/null; do /bin/sleep 0.1; "
                    @"done; %@; /usr/bin/open %@) &",
                    pid, preOpenCmd, quotedDestinationPath];

  [NSTask
      launchedTaskWithLaunchPath:@"/bin/sh"
                       arguments:[NSArray arrayWithObjects:@"-c", script, nil]];
}

bool AtomBundleMover::IsApplicationAtPathRunning(NSString* bundlePath) {
  bundlePath = [bundlePath stringByStandardizingPath];

  for (NSRunningApplication* runningApplication in
       [[NSWorkspace sharedWorkspace] runningApplications]) {
    NSString* runningAppBundlePath =
        [[[runningApplication bundleURL] path] stringByStandardizingPath];
    if ([runningAppBundlePath isEqualToString:bundlePath]) {
      return true;
    }
  }
  return false;
}

bool AtomBundleMover::Trash(NSString* path) {
  bool result = false;

  if (floor(NSAppKitVersionNumber) >= NSAppKitVersionNumber10_8) {
    result = [[NSFileManager defaultManager]
          trashItemAtURL:[NSURL fileURLWithPath:path]
        resultingItemURL:NULL
                   error:NULL];
  }

  if (!result) {
    result = [[NSWorkspace sharedWorkspace]
        performFileOperation:NSWorkspaceRecycleOperation
                      source:[path stringByDeletingLastPathComponent]
                 destination:@""
                       files:[NSArray arrayWithObject:[path lastPathComponent]]
                         tag:NULL];
  }

  // As a last resort try trashing with AppleScript.
  // This allows us to trash the app in macOS Sierra even when the app is
  // running inside an app translocation image.
  if (!result) {
    NSAppleScript* appleScript = [[[NSAppleScript alloc]
        initWithSource:
            [NSString
                stringWithFormat:
                    @"\
                    set theFile to POSIX file \"%@\" "
                    @"\n\
                       tell application \"Finder\" "
                    @"\n\
                        move theFile to trash \n\
   "
                    @"                   end tell",
                    path]] autorelease];
    NSDictionary* errorDict = nil;
    NSAppleEventDescriptor* scriptResult =
        [appleScript executeAndReturnError:&errorDict];
    result = (scriptResult != nil);
  }

  return result;
}

bool AtomBundleMover::DeleteOrTrash(NSString* path) {
  NSError* error;

  if ([[NSFileManager defaultManager] removeItemAtPath:path error:&error]) {
    return true;
  } else {
    return Trash(path);
  }
}

}  // namespace cocoa

}  // namespace ui

}  // namespace atom