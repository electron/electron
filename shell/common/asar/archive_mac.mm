// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "electron/fuses.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

namespace asar {

void Archive::MaybeValidateArchiveSignature() {
// This method uses a private API SecCodeValidateFileResource
// which should not be used in the MAS build
#ifndef MAS_BUILD
  // If the fuse is not enabled, don't waste any time here.
  if (!electron::fuses::IsEmbeddedAsarSignatureValidationEnabled())
    return;

  NSURL* bundle_url =
      [NSURL URLWithString:[[NSBundle mainBundle] executablePath]];
  // /Stuff/Electron.app/Contents/MacOS/Electron is the bundle url.
  NSURL* contents_url = [[bundle_url URLByDeletingLastPathComponent]
      URLByDeletingLastPathComponent];
  const std::string abs_path = path_.value();

  // If the path to the app.asar file is not contained by the bundle we can't
  // validate the signature.
  if (abs_path.rfind(base::SysNSStringToUTF8([contents_url absoluteString]),
                     0) != 0)
    return;

  SecStaticCodeRef ref = NULL;

  OSStatus status;
  status = SecStaticCodeCreateWithPath((CFURLRef)bundle_url, kSecCSDefaultFlags,
                                       &ref);

  if (ref == NULL) {
    LOG(FATAL) << "Failed to create static code reference for " << bundle_url;
    return;
  }

  if (status != noErr) {
    CFRelease(ref);
    LOG(FATAL) << "Failed to create static code reference for " << bundle_url;
    return;
  }

  NSString* archive_path = base::mac::FilePathToNSString(path_);
  NSString* relative_path =
      [archive_path substringFromIndex:[[contents_url absoluteString] length]];
  NSData* data = [NSData dataWithContentsOfFile:archive_path];
  status = SecCodeValidateFileResource(ref, (CFStringRef)relative_path,
                                       (CFDataRef)data, kSecCSDefaultFlags);

  if (status != noErr) {
    CFRelease(ref);
    LOG(FATAL) << "Failed to validate archive signature for " << relative_path
               << ". Got error: " << status;
    return;
  }

  CFRelease(ref);
#endif
}

}  // namespace asar
