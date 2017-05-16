// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

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

std::string MessageForOSStatus(OSStatus status, const char* default_message) {
  switch (status) {
    case kLSAppInTrashErr:
      return "The application cannot be run because it is inside a Trash "
             "folder.";
    case kLSUnknownErr:
      return "An unknown error has occurred.";
    case kLSNotAnApplicationErr:
      return "The item to be registered is not an application.";
    case kLSNotInitializedErr:
      return "Formerly returned by LSInit on initialization failure; "
             "no longer used.";
    case kLSDataUnavailableErr:
      return "Data of the desired type is not available (for example, there is "
             "no kind string).";
    case kLSApplicationNotFoundErr:
      return "No application in the Launch Services database matches the input "
             "criteria.";
    case kLSDataErr:
      return "Data is structured improperly (for example, an item’s "
             "information property list is malformed). Not used in macOS 10.4.";
    case kLSLaunchInProgressErr:
      return "A launch of the application is already in progress.";
    case kLSServerCommunicationErr:
      return "There is a problem communicating with the server process that "
             "maintains the Launch Services database.";
    case kLSCannotSetInfoErr:
      return "The filename extension to be hidden cannot be hidden.";
    case kLSIncompatibleSystemVersionErr:
      return "The application to be launched cannot run on the current Mac OS "
             "version.";
    case kLSNoLaunchPermissionErr:
      return "The user does not have permission to launch the application (on a"
             "managed network).";
    case kLSNoExecutableErr:
      return "The executable file is missing or has an unusable format.";
    case kLSNoClassicEnvironmentErr:
      return "The Classic emulation environment was required but is not "
             "available.";
    case kLSMultipleSessionsNotSupportedErr:
      return "The application to be launched cannot run simultaneously in two "
             "different user sessions.";
    default:
      return base::StringPrintf("%s (%d)", default_message, status);
  }
}

// This may be called from a global dispatch queue, the methods used here are
// thread safe, including LSGetApplicationForURL (> 10.2) and
// NSWorkspace#openURLs.
std::string OpenURL(NSURL* ns_url, bool activate) {
  CFURLRef openingApp = nullptr;
  OSStatus status = LSGetApplicationForURL(base::mac::NSToCFCast(ns_url),
                                           kLSRolesAll,
                                           nullptr,
                                           &openingApp);
  if (status != noErr)
    return MessageForOSStatus(status, "Failed to open");

  CFRelease(openingApp);  // NOT A BUG; LSGetApplicationForURL retains for us

  NSUInteger launchOptions = NSWorkspaceLaunchDefault;
  if (!activate)
    launchOptions |= NSWorkspaceLaunchWithoutActivation;

  bool opened = [[NSWorkspace sharedWorkspace]
                            openURLs:@[ns_url]
             withAppBundleIdentifier:nil
                             options:launchOptions
      additionalEventParamDescriptor:nil
                   launchIdentifiers:nil];
  if (!opened)
    return "Failed to open URL";

  return "";
}

}  // namespace

namespace platform_util {
  
void ShowItemInFolder(const base::FilePath& full_path) {
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
  status = AECreateList(nullptr,  // factoringPtr
                        0,  // factoredSize
                        false,  // isRecord
                        fileList.OutPointer());  // resultList
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status) << "Could not create OpenItem() AE file list";
    return false;
  }

  // Add the single path to the file list.  C-style cast to avoid both a
  // static_cast and a const_cast to get across the toll-free bridge.
  CFURLRef pathURLRef = base::mac::NSToCFCast(
      [NSURL fileURLWithPath:path_string]);
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
                  nullptr, // idleProc
                  nullptr);  // filterProc
  if (status != noErr) {
    OSSTATUS_LOG(WARNING, status)
        << "Could not send AE to Finder in OpenItem()";
  }
  return status == noErr;
}

bool OpenExternal(const GURL& url, bool activate) {
  DCHECK([NSThread isMainThread]);
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (ns_url)
    return OpenURL(ns_url, activate).empty();
  return false;
}

void OpenExternal(const GURL& url, bool activate,
                  const OpenExternalCallback& callback) {
  NSURL* ns_url = net::NSURLWithGURL(url);
  if (!ns_url) {
    callback.Run("Invalid URL");
    return;
  }

  __block OpenExternalCallback c = callback;
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    __block std::string error = OpenURL(ns_url, activate);
    dispatch_async(dispatch_get_main_queue(), ^{
      c.Run(error);
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

#if defined MAS_BUILD
void SetProcessTitleActivityMonitor(const std::string& name) {
  // NB: Can't be implemented with public APIs
}
#else
void SetProcessTitleActivityMonitor(const std::string& name) {
  if (name.length() < 1) {
    NOTREACHED() << "SetProcessTitleActivityMonitor given bad name.";
    return;
  }

  CFStringRef process_name = base::SysUTF8ToCFStringRef(name);

  // Private CFType used in these LaunchServices calls.
  typedef CFTypeRef PrivateLSASN;
  typedef PrivateLSASN (*LSGetCurrentApplicationASNType)();
  typedef OSStatus (*LSSetApplicationInformationItemType)(int, PrivateLSASN,
                                                          CFStringRef,
                                                          CFStringRef,
                                                          CFDictionaryRef*);

  static LSGetCurrentApplicationASNType ls_get_current_application_asn_func =
      NULL;
  static LSSetApplicationInformationItemType
      ls_set_application_information_item_func = NULL;
  static CFStringRef ls_display_name_key = NULL;

  static bool did_symbol_lookup = false;
  if (!did_symbol_lookup) {
    did_symbol_lookup = true;
    CFBundleRef launch_services_bundle =
        CFBundleGetBundleWithIdentifier(CFSTR("com.apple.LaunchServices"));
    if (!launch_services_bundle) {
      LOG(ERROR) << "Failed to look up LaunchServices bundle";
      return;
    }

    ls_get_current_application_asn_func =
        reinterpret_cast<LSGetCurrentApplicationASNType>(
            CFBundleGetFunctionPointerForName(
                launch_services_bundle, CFSTR("_LSGetCurrentApplicationASN")));
    if (!ls_get_current_application_asn_func)
      LOG(ERROR) << "Could not find _LSGetCurrentApplicationASN";

    ls_set_application_information_item_func =
        reinterpret_cast<LSSetApplicationInformationItemType>(
            CFBundleGetFunctionPointerForName(
                launch_services_bundle,
                CFSTR("_LSSetApplicationInformationItem")));
    if (!ls_set_application_information_item_func)
      LOG(ERROR) << "Could not find _LSSetApplicationInformationItem";

    CFStringRef* key_pointer = reinterpret_cast<CFStringRef*>(
        CFBundleGetDataPointerForName(launch_services_bundle,
                                      CFSTR("_kLSDisplayNameKey")));
    ls_display_name_key = key_pointer ? *key_pointer : NULL;
    if (!ls_display_name_key)
      LOG(ERROR) << "Could not find _kLSDisplayNameKey";

    // Internally, this call relies on the Mach ports that are started up by the
    // Carbon Process Manager.  In debug builds this usually happens due to how
    // the logging layers are started up; but in release, it isn't started in as
    // much of a defined order.  So if the symbols had to be loaded, go ahead
    // and force a call to make sure the manager has been initialized and hence
    // the ports are opened.
    ProcessSerialNumber psn;
    GetCurrentProcess(&psn);
  }
  if (!ls_get_current_application_asn_func ||
      !ls_set_application_information_item_func ||
      !ls_display_name_key) {
    return;
  }

  PrivateLSASN asn = ls_get_current_application_asn_func();

  // Constant used by WebKit; what exactly it means is unknown.
  const int magic_session_constant = -2;
  OSErr err =
      ls_set_application_information_item_func(magic_session_constant, asn,
                                               ls_display_name_key,
                                               process_name,
                                               NULL /* optional out param */);
  LOG_IF(ERROR, err) << "Call to set process name failed, err " << err;
}
#endif

}  // namespace platform_util
