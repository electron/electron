// Copyright 2023 Microsoft, Inc.
// Copyright 2013 The Chromium Authors
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/mac/codesign_util.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <mach/mach.h>
#include <unistd.h>

#include <optional>

#include "base/apple/foundation_util.h"
#include "base/apple/mach_logging.h"
#include "base/apple/osstatus_logging.h"
#include "base/apple/scoped_cftyperef.h"

namespace electron {

namespace {

std::optional<bool> IsUnsignedOrAdHocSigned(SecCodeRef code) {
  base::apple::ScopedCFTypeRef<SecStaticCodeRef> static_code;
  OSStatus status = SecCodeCopyStaticCode(code, kSecCSDefaultFlags,
                                          static_code.InitializeInto());
  if (status == errSecCSUnsigned) {
    return true;
  }
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopyStaticCode";
    return {};
  }
  // Copy the signing info from the SecStaticCodeRef.
  base::apple::ScopedCFTypeRef<CFDictionaryRef> signing_info;
  status =
      SecCodeCopySigningInformation(static_code.get(), kSecCSSigningInformation,
                                    signing_info.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopySigningInformation";
    return {};
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
    return {};
  }
  if (static_cast<uint32_t>(flags) & kSecCodeSignatureAdhoc) {
    return true;
  }
  return false;
}

}  // namespace

std::optional<audit_token_t> GetParentProcessAuditToken() {
  pid_t parent_pid = getppid();

  // task_name_for_pid() yields a name port (not a control port), so it does
  // not require any special entitlement. Same approach as Chromium's
  // remoting/host/webauthn/remote_webauthn_caller_security_utils.cc.
  task_name_t parent_task;
  kern_return_t kr =
      task_name_for_pid(mach_task_self(), parent_pid, &parent_task);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "task_name_for_pid";
    return std::nullopt;
  }

  audit_token_t token;
  mach_msg_type_number_t size = TASK_AUDIT_TOKEN_COUNT;
  kr = task_info(parent_task, TASK_AUDIT_TOKEN,
                 reinterpret_cast<task_info_t>(&token), &size);
  mach_port_deallocate(mach_task_self(), parent_task);
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "task_info(TASK_AUDIT_TOKEN)";
    return std::nullopt;
  }

  return token;
}

bool ProcessSignatureIsSameWithCurrentApp(audit_token_t audit_token) {
  // Get and check the code signature of current app.
  base::apple::ScopedCFTypeRef<SecCodeRef> self_code;
  OSStatus status =
      SecCodeCopySelf(kSecCSDefaultFlags, self_code.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopySelf";
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
  // Get the code signature of process. kSecGuestAttributeAudit is preferred
  // over kSecGuestAttributePid because the audit token is bound to a single
  // process instance and cannot be spoofed by PID reuse or by the target
  // exec()ing into a different image after the lookup.
  base::apple::ScopedCFTypeRef<CFDataRef> audit_token_cf(
      CFDataCreate(nullptr, reinterpret_cast<const UInt8*>(&audit_token),
                   sizeof(audit_token_t)));
  const void* attribute_keys[] = {kSecGuestAttributeAudit};
  const void* attribute_values[] = {audit_token_cf.get()};
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
    // If the code is unsigned, don't log that (it's not an actual error).
    if (status != errSecCSUnsigned) {
      OSSTATUS_LOG(ERROR, status) << "SecCodeCheckValidity";
    }
    return false;
  }
  return status == errSecSuccess;
}

}  // namespace electron
