// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include "base/cancelable_callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_aedesc.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

namespace {

NSError *PlatformError(NSString* message) {
  return [NSError errorWithDomain:@"Electron"
                  code:0
                  userInfo:@{NSLocalizedDescriptionKey: message}];
}

NSString *MessageForOSStatus(OSStatus status) {
  switch (status) {
    case kLSAppInTrashErr: return @"The application cannot be run because it is inside a Trash folder.";
    case kLSUnknownErr: return @"An unknown error has occurred.";
    case kLSNotAnApplicationErr: return @"The item to be registered is not an application.";
    case kLSNotInitializedErr: return @"Formerly returned by LSInit on initialization failure; no longer used.";
    case kLSDataUnavailableErr: return @"Data of the desired type is not available (for example, there is no kind string).";
    case kLSApplicationNotFoundErr: return @"No application in the Launch Services database matches the input criteria.";
    case kLSDataErr: return @"Data is structured improperly (for example, an item’s information property list is malformed). Not used in macOS 10.4.";
    case kLSLaunchInProgressErr: return @"A launch of the application is already in progress.";
    case kLSServerCommunicationErr: return @"There is a problem communicating with the server process that maintains the Launch Services database.";
    case kLSCannotSetInfoErr: return @"The filename extension to be hidden cannot be hidden.";
    case kLSIncompatibleSystemVersionErr: return @"The application to be launched cannot run on the current Mac OS version.";
    case kLSNoLaunchPermissionErr: return @"The user does not have permission to launch the application (on a managed network).";
    case kLSNoExecutableErr: return @"The executable file is missing or has an unusable format.";
    case kLSNoClassicEnvironmentErr: return @"The Classic emulation environment was required but is not available.";
    case kLSMultipleSessionsNotSupportedErr: return @"The application to be launched cannot run simultaneously in two different user sessions.";
    default: return [NSString stringWithFormat:@"Failed to open (%@)", @(status)];
  }
}

// This may be called from a global dispatch queue, the methods used here are thread safe,
// including LSGetApplicationForURL (> 10.2) and NSWorkspace#openURLs.
NSError* OpenURL(NSURL* ns_url, bool activate) {
  CFURLRef openingApp = NULL;
  OSStatus status = LSGetApplicationForURL((CFURLRef)ns_url,
                                           kLSRolesAll,
                                           NULL,
                                           &openingApp);
  if (status != noErr) {
    return PlatformError(MessageForOSStatus(status));
  }
  CFRelease(openingApp);  // NOT A BUG; LSGetApplicationForURL retains for us

  NSUInteger launchOptions = NSWorkspaceLaunchDefault;
  if (!activate)
    launchOptions |= NSWorkspaceLaunchWithoutActivation;

  bool opened = [[NSWorkspace sharedWorkspace] openURLs: @[ns_url]
                                               withAppBundleIdentifier: nil
                                               options: launchOptions
                                               additionalEventParamDescriptor: nil
                                               launchIdentifiers: nil];
  if (!opened) return PlatformError(@"Failed to open URL");
  return nil;
}

v8::Local<v8::Value> ConvertNSError(v8::Isolate* isolate, NSError* platformError) {
  if (!platformError) {
    return v8::Null(isolate);
  } else {
    v8::Local<v8::String> error_message =
      v8::String::NewFromUtf8(isolate, platformError.localizedDescription.UTF8String);
    return v8::Exception::Error(error_message);
  }
}

}  // namespace

namespace platform_util {

bool ShowItemInFolder(const base::FilePath& path) {
  // The API only takes absolute path.
  base::FilePath full_path =
      path.IsAbsolute() ? path : base::MakeAbsoluteFilePath(path);

  DCHECK([NSThread isMainThread]);
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string || ![[NSWorkspace sharedWorkspace] selectFile:path_string
                                        inFileViewerRootedAtPath:@""]) {
    LOG(WARNING) << "NSWorkspace failed to select file " << full_path.value();
    return false;
  }
  return true;
}

// This function opens a file.  This doesn't use LaunchServices or NSWorkspace
// because of two bugs:
//  1. Incorrect app activation with com.apple.quarantine:
//     http://crbug.com/32921
//  2. Silent no-op for unassociated file types: http://crbug.com/50263
// Instead, an AppleEvent is constructed to tell the Finder to open the
// document.
bool OpenItem(const base::FilePath& full_path) {
  DCHECK([NSThread isMainThread]);
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string)
    return false;

  // Create the target of this AppleEvent, the Finder.
  base::mac::ScopedAEDesc<AEAddressDesc> address;
  const OSType finderCreatorCode = 'MACS';
  OSErr status = AECreateDesc(typeApplSignature,  // type
                              &finderCreatorCode,  // data
                              sizeof(finderCreatorCode),  // dataSize
                              address.OutPointer());  // result
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE target";
    return false;
  }

  // Build the AppleEvent data structure that instructs Finder to open files.
  base::mac::ScopedAEDesc<AppleEvent> theEvent;
  status = AECreateAppleEvent(kCoreEventClass,  // theAEEventClass
                              kAEOpenDocuments,  // theAEEventID
                              address,  // target
                              kAutoGenerateReturnID,  // returnID
                              kAnyTransactionID,  // transactionID
                              theEvent.OutPointer());  // result
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE event";
    return false;
  }

  // Create the list of files (only ever one) to open.
  base::mac::ScopedAEDesc<AEDescList> fileList;
  status = AECreateList(NULL,  // factoringPtr
                        0,  // factoredSize
                        false,  // isRecord
                        fileList.OutPointer());  // resultList
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE file list";
    return false;
  }

  // Add the single path to the file list.  C-style cast to avoid both a
  // static_cast and a const_cast to get across the toll-free bridge.
  CFURLRef pathURLRef = (CFURLRef)[NSURL fileURLWithPath:path_string];
  FSRef pathRef;
  if (CFURLGetFSRef(pathURLRef, &pathRef)) {
    status = AEPutPtr(fileList.OutPointer(),  // theAEDescList
                      0,  // index
                      typeFSRef,  // typeCode
                      &pathRef,  // dataPtr
                      sizeof(pathRef));  // dataSize
    if (status != noErr) {
      OSSTATUS_LOG(WARNING, status)
          << "Could not add file path to AE list in OpenItem()";
      return false;
    }
  } else {
    LOG(WARNING) << "Could not get FSRef for path URL in OpenItem()";
    return false;
  }

  // Attach the file list to the AppleEvent.
  status = AEPutParamDesc(theEvent.OutPointer(),  // theAppleEvent
                          keyDirectObject,  // theAEKeyword
                          fileList);  // theAEDesc
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status)
        << "Could not put the AE file list the path in OpenItem()";
    return false;
  }

  // Send the actual event.  Do not care about the reply.
  base::mac::ScopedAEDesc<AppleEvent> reply;
  status = AESend(theEvent,  // theAppleEvent
                  reply.OutPointer(),  // reply
                  kAENoReply + kAEAlwaysInteract,  // sendMode
                  kAENormalPriority,  // sendPriority
                  kAEDefaultTimeout,  // timeOutInTicks
                  NULL, // idleProc
                  NULL);  // filterProc
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status)
        << "Could not send AE to Finder in OpenItem()";
  }
  return status == noErr;
}

bool OpenExternal(const GURL& url, bool activate) {
  DCHECK([NSThread isMainThread]);
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (!ns_url) {
    return false;
  }
  NSError *error = OpenURL(ns_url, activate);
  return !error;
}

void OpenExternal(const GURL& url, bool activate, const OpenExternalCallback& c) {
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (!ns_url) {
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    c.Run(ConvertNSError(isolate, PlatformError(@"Invalid URL")));
    return;
  }

  __block OpenExternalCallback callback = c;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    NSError *openError = OpenURL(ns_url, activate);
    dispatch_async(dispatch_get_main_queue(), ^{
      v8::Isolate* isolate = v8::Isolate::GetCurrent();
      callback.Run(ConvertNSError(isolate, openError));
    });
  });
}

bool MoveItemToTrash(const base::FilePath& full_path) {
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  BOOL status = [[NSFileManager defaultManager]
                trashItemAtURL:[NSURL fileURLWithPath:path_string]
                resultingItemURL:nil
                error:nil];
  if (!path_string || !status)
    LOG(WARNING) << "NSWorkspace failed to move file " << full_path.value()
                 << " to trash";
  return status;
}

void Beep() {
  NSBeep();
}

}  // namespace platform_util
