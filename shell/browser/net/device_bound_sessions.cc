// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/device_bound_sessions.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "components/unexportable_keys/background_task_origin.h"
#include "components/unexportable_keys/features.h"
#include "components/unexportable_keys/unexportable_key_service_impl.h"
#include "components/unexportable_keys/unexportable_key_task_manager.h"
#include "crypto/unexportable_key.h"
#include "electron/fuses.h"

#if BUILDFLAG(IS_MAC)
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <array>
#include <cstdint>
#include <string>

#include "base/apple/foundation_util.h"
#include "base/apple/osstatus_logging.h"
#include "base/apple/scoped_cftyperef.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "crypto/hash.h"
#endif  // BUILDFLAG(IS_MAC)

namespace electron {

namespace {

bool IsSoftwareKeysForManualTestingEnabled() {
  return base::FeatureList::IsEnabled(
      unexportable_keys::
          kEnableBoundSessionCredentialsSoftwareKeysForManualTesting);
}

#if BUILDFLAG(IS_MAC)
// Returns "<TeamID>.<BundleID>.unexportable-keys" for the running app, or an
// empty string when it cannot be derived. macOS requires the binary to be
// codesigned with a matching `keychain-access-groups` entitlement, so an
// unsigned or ad-hoc signed build has no usable group.
std::string GetKeychainAccessGroup() {
  CFBundleRef main_bundle = CFBundleGetMainBundle();
  CFStringRef bundle_id =
      main_bundle ? CFBundleGetIdentifier(main_bundle) : nullptr;
  if (!bundle_id) {
    return {};
  }

  base::apple::ScopedCFTypeRef<SecCodeRef> self_code;
  OSStatus status =
      SecCodeCopySelf(kSecCSDefaultFlags, self_code.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopySelf";
    return {};
  }

  base::apple::ScopedCFTypeRef<SecStaticCodeRef> static_code;
  status = SecCodeCopyStaticCode(self_code.get(), kSecCSDefaultFlags,
                                 static_code.InitializeInto());
  if (status != errSecSuccess) {
    // Unsigned builds are the common development case, not an error.
    if (status != errSecCSUnsigned) {
      OSSTATUS_LOG(ERROR, status) << "SecCodeCopyStaticCode";
    }
    return {};
  }

  base::apple::ScopedCFTypeRef<CFDictionaryRef> signing_info;
  status =
      SecCodeCopySigningInformation(static_code.get(), kSecCSSigningInformation,
                                    signing_info.InitializeInto());
  if (status != errSecSuccess) {
    OSSTATUS_LOG(ERROR, status) << "SecCodeCopySigningInformation";
    return {};
  }

  // Absent for unsigned, ad-hoc signed, and self-signed binaries.
  CFStringRef team_id = base::apple::GetValueFromDictionary<CFStringRef>(
      signing_info.get(), kSecCodeInfoTeamIdentifier);
  if (!team_id) {
    return {};
  }

  return base::StrCat({base::SysCFStringRefToUTF8(team_id), ".",
                       base::SysCFStringRefToUTF8(bundle_id),
                       ".unexportable-keys"});
}
#endif  // BUILDFLAG(IS_MAC)

// Returns the key provider configuration for the browser context rooted at
// `context_path`, or nullopt when this app cannot use unexportable keys at all.
std::optional<crypto::UnexportableKeyProvider::Config> GetKeyProviderConfig(
    [[maybe_unused]] const base::FilePath& context_path) {
  crypto::UnexportableKeyProvider::Config config;
#if BUILDFLAG(IS_MAC)
  config.keychain_access_group = GetKeychainAccessGroup();
  if (config.keychain_access_group.empty()) {
    // crypto CHECKs on an empty access group, and a group the app is not
    // entitled to would fail anyway. The mock software provider used for
    // manual testing never touches the keychain, so it is still usable.
    if (!IsSoftwareKeysForManualTestingEnabled()) {
      LOG(WARNING) << "Device Bound Sessions are enabled but no keychain "
                      "access group could be derived for this app, so no keys "
                      "are available. The app must be code signed with a team "
                      "identifier and hold a keychain-access-groups "
                      "entitlement for <TeamID>.<BundleID>.unexportable-keys.";
      return std::nullopt;
    }
  } else {
    // Scope this context's keys so that another context's garbage collector
    // does not delete them. `context_path` is unique per session: the default
    // session uses the user data dir, `persist:` partitions live under
    // Partitions/.
    const std::array<uint8_t, crypto::hash::kSha256Size> context_hash =
        crypto::hash::Sha256(context_path.value());
    config.application_tag = base::StrCat(
        {config.keychain_access_group, ".",
         base::HexEncodeLower(base::span(context_hash).first(8u)), ".dbsc"});
  }
#endif  // BUILDFLAG(IS_MAC)
  return config;
}

// The task manager is stateless scheduling machinery, so every context's
// service can share one. It must outlive them all.
unexportable_keys::UnexportableKeyTaskManager& GetSharedTaskManager() {
  static base::NoDestructor<unexportable_keys::UnexportableKeyTaskManager>
      instance;
  return *instance;
}

}  // namespace

bool ShouldEnableDeviceBoundSessions() {
  // The fuse is the production switch. With the fuse off, the software-keys
  // testing feature also turns DBSC on: mock keys are the only way to exercise
  // it in development on Linux, on unsigned macOS builds, and on Windows
  // machines without a TPM 2.0. A fused app never reaches this branch because
  // InitializeFeatureList() force-disables that feature.
  return fuses::IsDeviceBoundSessionsEnabled() ||
         IsSoftwareKeysForManualTestingEnabled();
}

std::unique_ptr<unexportable_keys::UnexportableKeyService>
CreateDeviceBoundSessionsKeyService(const base::FilePath& context_path) {
  std::optional<crypto::UnexportableKeyProvider::Config> config =
      GetKeyProviderConfig(context_path);
  if (!config ||
      !unexportable_keys::UnexportableKeyServiceImpl::
          IsUnexportableKeyProviderSupported(*config)) {
    return nullptr;
  }

  return std::make_unique<unexportable_keys::UnexportableKeyServiceImpl>(
      GetSharedTaskManager(),
      unexportable_keys::BackgroundTaskOrigin::kDeviceBoundSessionCredentials,
      std::move(*config));
}

}  // namespace electron
