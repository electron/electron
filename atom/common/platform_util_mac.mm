// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/mac/mac_logging.h"
#include "base/mac/scoped_aedesc.h"
#include "base/strings/sys_string_conversions.h"
#include "net/base/mac/url_conversions.h"
#include "url/gurl.h"

namespace platform_util {

void ShowItemInFolder(const base::FilePath& path) {
  // The API only takes absolute path.
  base::FilePath full_path =
      path.IsAbsolute() ? path : base::MakeAbsoluteFilePath(path);

  DCHECK([NSThread isMainThread]);
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string || ![[NSWorkspace sharedWorkspace] selectFile:path_string
                                        inFileViewerRootedAtPath:@""])
    LOG(WARNING) << "NSWorkspace failed to select file " << full_path.value();
}

// This function opens a file.  This doesn't use LaunchServices or NSWorkspace
// because of two bugs:
//  1. Incorrect app activation with com.apple.quarantine:
//     http://crbug.com/32921
//  2. Silent no-op for unassociated file types: http://crbug.com/50263
// Instead, an AppleEvent is constructed to tell the Finder to open the
// document.
void OpenItem(const base::FilePath& full_path) {
  DCHECK([NSThread isMainThread]);
  NSString* path_string = base::SysUTF8ToNSString(full_path.value());
  if (!path_string)
    return;

  // Create the target of this AppleEvent, the Finder.
  base::mac::ScopedAEDesc<AEAddressDesc> address;
  const OSType finderCreatorCode = 'MACS';
  OSErr status = AECreateDesc(typeApplSignature,  // type
                              &finderCreatorCode,  // data
                              sizeof(finderCreatorCode),  // dataSize
                              address.OutPointer());  // result
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE target";
    return;
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
    return;
  }

  // Create the list of files (only ever one) to open.
  base::mac::ScopedAEDesc<AEDescList> fileList;
  status = AECreateList(NULL,  // factoringPtr
                        0,  // factoredSize
                        false,  // isRecord
                        fileList.OutPointer());  // resultList
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE file list";
    return;
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
      return;
    }
  } else {
    LOG(WARNING) << "Could not get FSRef for path URL in OpenItem()";
    return;
  }

  // Attach the file list to the AppleEvent.
  status = AEPutParamDesc(theEvent.OutPointer(),  // theAppleEvent
                          keyDirectObject,  // theAEKeyword
                          fileList);  // theAEDesc
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status)
        << "Could not put the AE file list the path in OpenItem()";
    return;
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
}

bool OpenExternal(const GURL& url, bool activate) {
  DCHECK([NSThread isMainThread]);
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (!ns_url) {
    return false;
  }

  CFURLRef openingApp = NULL;
  OSStatus status = LSGetApplicationForURL((CFURLRef)ns_url,
                                           kLSRolesAll,
                                           NULL,
                                           &openingApp);
  if (status != noErr) {
    return false;
  }
  CFRelease(openingApp);  // NOT A BUG; LSGetApplicationForURL retains for us

  NSUInteger launchOptions = NSWorkspaceLaunchDefault;
  if (!activate)
    launchOptions |= NSWorkspaceLaunchWithoutActivation;

  return [[NSWorkspace sharedWorkspace] openURLs: @[ns_url]
                                        withAppBundleIdentifier: nil
                                        options: launchOptions
                                        additionalEventParamDescriptor: NULL
                                        launchIdentifiers: NULL];
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
