// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/mac/codesign_util.h"

#include "base/apple/osstatus_logging.h"
#include "base/apple/scoped_cftyperef.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace electron {

bool ProcessBelongToCurrentApp(pid_t pid) {
  // Get and check the code signature of current app.
  base::apple::ScopedCFTypeRef<SecCodeRef> self_code;
  OSStatus status =
      SecCodeCopySelf(kSecCSDefaultFlags, self_code.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyGuestWithAttributes";
    return false;
  }
  // Get the code signature of process.
  base::apple::ScopedCFTypeRef<CFNumberRef> process_cf(
      CFNumberCreate(nullptr, kCFNumberIntType, &pid));
  const void* attribute_keys[] = {kSecGuestAttributePid};
  const void* attribute_values[] = {process_cf.get()};
  base::apple::ScopedCFTypeRef<CFDictionaryRef> attributes(CFDictionaryCreate(
      nullptr, attribute_keys, attribute_values, std::size(attribute_keys),
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
  base::apple::ScopedCFTypeRef<SecCodeRef> process_code;
  status = SecCodeCopyGuestWithAttributes(nullptr, attributes.get(),
                                          kSecCSDefaultFlags,
                                          process_code.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyGuestWithAttributes";
    return false;
  }
  // Get the requirement of current app's code signature.
  base::apple::ScopedCFTypeRef<SecRequirementRef> self_requirement;
  status = SecCodeCopyDesignatedRequirement(self_code.get(), kSecCSDefaultFlags,
                                            self_requirement.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyDesignatedRequirement";
    return false;
  }
  // Check whether the process meets the signature requirement of current app.
  status = SecCodeCheckValidity(process_code.get(), kSecCSDefaultFlags,
                                self_requirement.get());
  return status == errSecSuccess;
}

}  // namespace electron
