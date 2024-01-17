// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/mac/codesign_util.h"

#include <optional>

#include "base/apple/foundation_util.h"
#include "base/apple/osstatus_logging.h"
#include "base/apple/scoped_cftyperef.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace electron {

std::optional<bool> IsUnsignedOrAdHocSigned(SecCodeRef code) {
  base::apple::ScopedCFTypeRef<SecStaticCodeRef> static_code;
  OSStatus status = SecCodeCopyStaticCode(code, kSecCSDefaultFlags,
                                          static_code.InitializeInto());
  if (status == errSecCSUnsigned) {
    return true;
  }
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyStaticCode";
    return std::optional<bool>();
  }
  // Copy the signing info from the SecStaticCodeRef.
  base::apple::ScopedCFTypeRef<CFDictionaryRef> signing_info;
  status =
      SecCodeCopySigningInformation(static_code.get(), kSecCSSigningInformation,
                                    signing_info.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopySigningInformation";
    return std::optional<bool>();
  }
  // Look up the code signing flags. If the flags are absent treat this as
  // unsigned. This decision is consistent with the StaticCode source:
  // https://github.com/apple-oss-distributions/Security/blob/Security-60157.40.30.0.1/OSX/libsecurity_codesigning/lib/StaticCode.cpp#L2270
  CFNumberRef signing_info_flags =
      base::apple::GetValueFromDictionary<CFNumberRef>(signing_info.get(),
                                                       kSecCodeInfoFlags);
  if (!signing_info_flags) {
    return true;
  }
  // Using a long long to extract the value from the CFNumberRef to be
  // consistent with how it was packed by Security.framework.
  // https://github.com/apple-oss-distributions/Security/blob/Security-60157.40.30.0.1/OSX/libsecurity_utilities/lib/cfutilities.h#L262
  long long flags;
  if (!CFNumberGetValue(signing_info_flags, kCFNumberLongLongType, &flags)) {
    LOG(ERROR) << "CFNumberGetValue";
    return std::optional<bool>();
  }
  if (static_cast<uint32_t>(flags) & kSecCodeSignatureAdhoc) {
    return true;
  }
  return false;
}

bool ProcessSignatureIsSameWithCurrentApp(pid_t pid) {
  // Get and check the code signature of current app.
  base::apple::ScopedCFTypeRef<SecCodeRef> self_code;
  OSStatus status =
      SecCodeCopySelf(kSecCSDefaultFlags, self_code.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyGuestWithAttributes";
    return false;
  }
  std::optional<bool> not_signed = IsUnsignedOrAdHocSigned(self_code.get());
  if (!not_signed.has_value()) {
    // Error happened.
    return false;
  }
  if (not_signed.value()) {
    // Current app is not signed.
    return true;
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
  DCHECK(self_requirement.get());
  // Check whether the process meets the signature requirement of current app.
  status = SecCodeCheckValidity(process_code.get(), kSecCSDefaultFlags,
                                self_requirement.get());
  if (status != errSecSuccess && status != errSecCSReqFailed) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCheckValidity";
    return false;
  }
  return status == errSecSuccess;
}

}  // namespace electron
