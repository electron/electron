// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <iomanip>
#include <string>

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "shell/common/asar/asar_util.h"

namespace asar {

absl::optional<base::FilePath> Archive::RelativePath() const {
  base::FilePath bundle_path = base::mac::MainBundlePath().Append("Contents");

  base::FilePath relative_path;
  if (!bundle_path.AppendRelativePath(path_, &relative_path))
    return absl::nullopt;

  return relative_path;
}

absl::optional<IntegrityPayload> Archive::HeaderIntegrity() const {
  absl::optional<base::FilePath> relative_path = RelativePath();
  // Callers should have already asserted this
  CHECK(relative_path.has_value());

  NSDictionary* integrity = [[NSBundle mainBundle]
      objectForInfoDictionaryKey:@"ElectronAsarIntegrity"];

  // Integrity not provided
  if (!integrity)
    return absl::nullopt;

  NSString* ns_relative_path =
      base::mac::FilePathToNSString(relative_path.value());

  NSDictionary* integrity_payload = [integrity objectForKey:ns_relative_path];

  if (!integrity_payload)
    return absl::nullopt;

  NSString* algorithm = [integrity_payload objectForKey:@"algorithm"];
  NSString* hash = [integrity_payload objectForKey:@"hash"];
  if (algorithm && hash && [algorithm isEqualToString:@"SHA256"]) {
    IntegrityPayload header_integrity;
    header_integrity.algorithm = HashAlgorithm::SHA256;
    header_integrity.hash = base::SysNSStringToUTF8(hash);
    return header_integrity;
  }

  return absl::nullopt;
}

}  // namespace asar
