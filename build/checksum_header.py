#!/usr/bin/env python3

import os
import sys
import hashlib

dir_path = os.path.dirname(os.path.realpath(__file__))

TEMPLATE_H = """
#ifndef ELECTRON_SNAPSHOT_CHECKSUM_H_
#define ELECTRON_SNAPSHOT_CHECKSUM_H_

namespace electron::snapshot_checksum {

inline constexpr std::string_view kChecksum = "{checksum}";

}  // namespace electron::snapshot_checksum

#endif  // ELECTRON_SNAPSHOT_CHECKSUM_H_
"""

def calculate_sha256(filepath):
    sha256_hash = hashlib.sha256()
    with open(filepath, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

input_file = sys.argv[1]
output_file = sys.argv[2]

checksum = calculate_sha256(input_file)

checksum_h = TEMPLATE_H.replace("{checksum}", checksum)

with open(output_file, 'w') as f:
    f.write(checksum_h)
