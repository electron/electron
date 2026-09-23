#!/usr/bin/env python3

"""Prints a short, stable fingerprint of a patch directory.

Run at `gn gen` time from build/args/all.gn to salt V8's code-cache version
hash with the exact set of patches applied on top of the V8 release, so that
code caches and serialized wasm modules written by a differently-patched build
of the same V8 version are rejected instead of reused.
"""

import hashlib
import os
import sys


def patches_hash(patch_dir):
  digest = hashlib.sha256()
  with open(os.path.join(patch_dir, '.patches'), encoding='utf-8') as f:
    patch_names = [line.strip() for line in f if line.strip()]
  for patch_name in patch_names:
    digest.update(patch_name.encode('utf-8') + b'\n')
    with open(os.path.join(patch_dir, patch_name), 'rb') as patch:
      # Normalise line endings so a CRLF checkout produces the same salt.
      digest.update(patch.read().replace(b'\r\n', b'\n'))
  return digest.hexdigest()[:16]


if __name__ == '__main__':
  print(patches_hash(sys.argv[1]))
