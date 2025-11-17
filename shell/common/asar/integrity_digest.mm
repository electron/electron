// Copyright (c) 2025 Noah Gregory
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/integrity_digest.h"

#include <Foundation/Foundation.h>

#include <array>
#include <cstdint>
#include <span>

#include "base/strings/sys_string_conversions.h"
#include "crypto/hash.h"

namespace asar {

constexpr crypto::hash::HashKind kIntegrityDictionaryHashKind =
    crypto::hash::HashKind::kSha256;

constexpr size_t kIntegrityDictionaryDigestSize =
    DigestSizeForHashKind(kIntegrityDictionaryHashKind);
constexpr char kIntegrityDictionaryDigestSentinel[] =
    "AGbevlPCksUGKNL8TSn7wGmJEuJsXb2A";

struct IntegrityDictionaryDigestSlot {
  uint8_t sentinel[sizeof(kIntegrityDictionaryDigestSentinel) - 1];
  uint8_t used;
  uint8_t version;
  uint8_t digest[kIntegrityDictionaryDigestSize];
};

constexpr IntegrityDictionaryDigestSlot MakeDigestSlot(
    const char (&sentinel)[33]) {
  IntegrityDictionaryDigestSlot slot{};
  std::span<uint8_t, 32> slot_sentinel_span(slot.sentinel);
  std::copy_n(sentinel, slot_sentinel_span.size(), slot_sentinel_span.begin());
  slot.used = false;  // to be set at package-time
  slot.version = 0;   // to be set at package-time
  return slot;
}

__attribute__((section("__DATA_CONST,__asar_integrity"), used))
const volatile IntegrityDictionaryDigestSlot kIntegrityDictionaryDigest =
    MakeDigestSlot(kIntegrityDictionaryDigestSentinel);

bool IsIntegrityDictionaryValid(NSDictionary* integrity) {
  if (kIntegrityDictionaryDigest.used == false)
    return true;  // No digest to validate against, fail open
  if (kIntegrityDictionaryDigest.version != 1)
    return false;  // Unknown version, fail closed
  crypto::hash::Hasher integrity_hasher(kIntegrityDictionaryHashKind);
  for (NSString *relative_path_key in
       [[integrity allKeys] sortedArrayUsingComparator:^NSComparisonResult(
                                NSString* s1, NSString* s2) {
         return [s1 compare:s2 options:NSLiteralSearch];
       }]) {
    NSDictionary* file_integrity = [integrity objectForKey:relative_path_key];
    NSString* algorithm = [file_integrity objectForKey:@"algorithm"];
    NSString* hash = [file_integrity objectForKey:@"hash"];
    integrity_hasher.Update(base::SysNSStringToUTF8(relative_path_key));
    integrity_hasher.Update(base::SysNSStringToUTF8(algorithm));
    integrity_hasher.Update(base::SysNSStringToUTF8(hash));
  }
  std::array<uint8_t, kIntegrityDictionaryDigestSize> digest;
  integrity_hasher.Finish(digest);
  if (!std::equal(digest.begin(), digest.end(),
                  kIntegrityDictionaryDigest.digest)) {
    return false;
  }
  return true;
}

}  // namespace asar
