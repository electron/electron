// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <string>

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "electron/fuses.h"

namespace asar {

void Archive::MaybeValidateArchiveSignature() {
// This method uses a private API SecCodeValidateFileResource
// which should not be used in the MAS build
#ifndef MAS_BUILD
  // If the fuse is not enabled, don't waste any time here.
  if (!electron::fuses::IsEmbeddedAsarSignatureValidationEnabled())
    return;

  NSURL* exec_url =
      [NSURL URLWithString:[[NSBundle mainBundle] executablePath]];
  NSURL* bundle_url = [[[NSBundle mainBundle] bundleURL]
      URLByAppendingPathComponent:@"Contents"];

  // If the path to the app.asar file is not contained by the bundle we can't
  // validate the signature.
  base::FilePath relative_path;
  if (!base::mac::NSStringToFilePath([bundle_url path])
           .AppendRelativePath(path_, &relative_path))
    return;

  base::ScopedCFTypeRef<SecStaticCodeRef> ref;

  OSStatus status;
  status = SecStaticCodeCreateWithPath((CFURLRef)exec_url, kSecCSDefaultFlags,
                                       ref.InitializeInto());

  if (!ref) {
    LOG(FATAL) << "Failed to create static code reference for " << exec_url;
    return;
  }

  if (status != noErr) {
    LOG(FATAL) << "Failed to create static code reference for " << exec_url;
    return;
  }

  NSString* ns_archive_path = base::mac::FilePathToNSString(path_);
  NSString* ns_relative_path = base::mac::FilePathToNSString(relative_path);
  NSData* data = [NSData dataWithContentsOfFile:ns_archive_path];
  status = SecCodeValidateFileResource(ref, (CFStringRef)ns_relative_path,
                                       (CFDataRef)data, kSecCSDefaultFlags);

  if (status != noErr) {
    LOG(FATAL) << "Failed to validate archive signature for " << relative_path
               << ". Got error: " << status;
    return;
  }
#endif
}

}  // namespace asar
