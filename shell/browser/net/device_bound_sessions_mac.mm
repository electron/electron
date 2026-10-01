// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/device_bound_sessions.h"

#import <Foundation/Foundation.h>

#include <string>
#include <string_view>

#include "base/apple/bundle_locations.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "crypto/apple/keychain_util.h"
#include "crypto/hash.h"
#include "shell/common/mac/codesign_util.h"

namespace electron {

namespace {

constexpr char kAccessGroupSuffix[] = ".unexportable-keys";

// The same purpose string Chrome tags its DBSC keys with, so that all of this
// app's DBSC keys share a prefix that can be queried or deleted together.
constexpr char kKeyPurpose[] = "dbsc-standard";

constexpr char kDocsUrl[] =
    "https://www.electronjs.org/docs/latest/tutorial/fuses#deviceboundsessions";

// Returns the first 64 bits of the SHA-256 hash of `data` as lowercase hex.
std::string HexEncodeLowerSha64(std::string_view data) {
  return base::HexEncodeLower(
      base::as_byte_span(crypto::hash::Sha256(data)).first<8>());
}

// Returns the keychain access group this app stores DBSC keys under,
// <TeamID>.<BundleID>.unexportable-keys, or std::nullopt if the app is not
// set up to use it. Every reason is logged, because from the web page's point
// of view DBSC then silently does nothing.
std::optional<std::string> ComputeAccessGroup(bool software_keys) {
  NSString* bundle_id = base::apple::MainBundle().bundleIdentifier;
  const std::string bundle =
      bundle_id ? base::SysNSStringToUTF8(bundle_id) : std::string();
  const std::optional<std::string> team = GetCurrentAppTeamIdentifier();

  if (software_keys) {
    // Mock keys never reach the Keychain, so an unsigned development build is
    // fine and the group is only a label.
    return base::StrCat({team.value_or("unsigned"), ".",
                         bundle.empty() ? "electron" : bundle,
                         kAccessGroupSuffix});
  }

  if (CurrentAppIsUnsignedOrAdHocSigned().value_or(false)) {
    LOG(ERROR) << "Device Bound Sessions are disabled: the app is unsigned or "
                  "ad-hoc signed, and hardware-backed keys need a real code "
                  "signature. See "
               << kDocsUrl;
    return std::nullopt;
  }
  if (!team) {
    LOG(ERROR) << "Device Bound Sessions are disabled: the app's code "
                  "signature has no Team ID. See "
               << kDocsUrl;
    return std::nullopt;
  }
  if (bundle.empty()) {
    LOG(ERROR) << "Device Bound Sessions are disabled: the app has no bundle "
                  "identifier. See "
               << kDocsUrl;
    return std::nullopt;
  }

  std::string group = base::StrCat({*team, ".", bundle, kAccessGroupSuffix});
  if (!crypto::apple::ExecutableHasKeychainAccessGroupEntitlement(group)) {
    LOG(ERROR) << "Device Bound Sessions are disabled: the app is not signed "
                  "with a keychain-access-groups entitlement containing \""
               << group << "\". See " << kDocsUrl;
    return std::nullopt;
  }
  return group;
}

}  // namespace

std::optional<crypto::UnexportableKeyProvider::Config>
GetDeviceBoundSessionsKeyProviderConfig(const base::FilePath& partition_path,
                                        bool software_keys) {
  // Whether mock keys are on is fixed for the life of the process, and the
  // access group is the same for every partition, so work it out and log any
  // problem only once.
  static const base::NoDestructor<std::optional<std::string>> access_group(
      ComputeAccessGroup(software_keys));
  if (!access_group->has_value()) {
    return std::nullopt;
  }

  crypto::UnexportableKeyProvider::Config config;
  config.keychain_access_group = **access_group;
  // Keys are tied to a partition's on-disk session store, so scope them to it.
  // Apple platforms keep key metadata in the Keychain rather than with the
  // wrapped key, and the tag is how this partition's keys are found again.
  config.application_tag = base::StrCat(
      {config.keychain_access_group, ".",
       HexEncodeLowerSha64(partition_path.value()), ".", kKeyPurpose});
  return config;
}

}  // namespace electron
