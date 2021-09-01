// Copyright (c) 2021 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/asar_util.h"

#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include <iomanip>
#include <string>

#include "base/logging.h"
#include "shell/common/asar/archive.h"

namespace asar {

void ValidateIntegrityOrDie(const char* data,
                            size_t size,
                            const IntegrityPayload& integrity) {
  if (integrity.algorithm == HashAlgorithm::SHA256) {
    unsigned char result[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(data, (CC_LONG)size, result);

    std::stringstream computed_hash;
    computed_hash << std::hex;

    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
      computed_hash << std::setw(2) << std::setfill('0') << (int)result[i];
    }

    if (integrity.hash != computed_hash.str()) {
      LOG(FATAL) << "Integrity check failed for asar archive ("
                 << integrity.hash << " vs " << computed_hash.str() << ")";
    }
  } else {
    LOG(FATAL) << "Unsupported hashing algorithm in ValidateIntegrityOrDie";
  }
}

}  // namespace asar
