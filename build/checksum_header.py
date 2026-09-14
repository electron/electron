#!/usr/bin/env python3

import os
import sys
import hashlib

dir_path = os.path.dirname(os.path.realpath(__file__))

TEMPLATE_H = """
#ifndef ELECTRON_SNAPSHOT_CHECKSUM_H_
#define ELECTRON_SNAPSHOT_CHECKSUM_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace electron::snapshot_checksum {

// SHA-256 of the v8 context snapshot blob this build ships.
inline constexpr std::string_view kChecksum = "{checksum}";

// Its size and leading bytes -- V8's blob header, which carries V8's own
// checksums of the rest of the blob -- for telling cheaply (no hashing) whether
// the blob loaded at runtime is the one that was built or a custom one.
inline constexpr size_t kSize = {size};
inline constexpr uint8_t kHeaderPrefix[] = {{header_prefix}};

}  // namespace electron::snapshot_checksum

#endif  // ELECTRON_SNAPSHOT_CHECKSUM_H_
"""

HEADER_PREFIX_LENGTH = 96

def calculate_sha256(filepath):
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

input_file = sys.argv[1]
output_file = sys.argv[2]

checksum = calculate_sha256(input_file)
size = os.path.getsize(input_file)
with open(input_file, "rb") as f:
    header_prefix = f.read(HEADER_PREFIX_LENGTH)

checksum_h = TEMPLATE_H.replace("{checksum}", checksum)
checksum_h = checksum_h.replace("{size}", str(size))
checksum_h = checksum_h.replace("{header_prefix}",
                                ", ".join(str(b) for b in header_prefix))

with open(output_file, 'w') as f:
    f.write(checksum_h)
