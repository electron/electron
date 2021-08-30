#!/usr/bin/env python

from __future__ import print_function
import pprint
import sys

def main(filename, target_cpu):
  variables = {
    # Electron specific variables:
    'built_with_electron': 1,
    # Used by node-gyp but not defined in common.gypi:
    'build_v8_with_gn': 'false',
    'enable_lto': 'false',
    'llvm_version': '14.0',
  }
  # Align to Chromium's build configurations. Without this, native node modules
  # will have different (wrong) ideas about how v8 structs are laid out in memory
  # on 64-bit machines, and will summarily fail to work.
  if target_cpu == 'arm64' or target_cpu == 'x64':
    variables['v8_enable_pointer_compression'] = 1

  output = { 'variables': variables }
  with open(filename, 'w+') as f:
    f.write(pprint.pformat(output, indent=2))

if __name__ == '__main__':
  sys.exit(main(*sys.argv[1:]))
