#!/usr/bin/env python3
# Copyright (c) 2026 Anthropic, PBC.
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

"""Writes the grit resource allowlist for a linked Mach-O binary.

grit's --allowlist-support makes every resource-macro use instantiate
ui::AllowlistedResource<id>(), which is __attribute__((used)) and so keeps a
symbol in the linked image (ICF folds the bodies, not the names). This lists
those symbols with llvm-nm and writes the ids in the format
//tools/resources/generate_resource_allowlist.py produces, one per line.
Point it at the dSYM's DWARF companion when the binary itself is stripped.
"""

import argparse
import re
import subprocess
import sys

_MARKER = re.compile(rb'__ZN2ui19AllowlistedResourceILi(\d+)EEEvv')


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--nm', required=True)
  parser.add_argument('--input', required=True)
  parser.add_argument('--output', required=True)
  args = parser.parse_args()

  symbols = subprocess.run(
      [args.nm, '--defined-only', '--format=just-symbols', args.input],
      check=True, stdout=subprocess.PIPE).stdout
  ids = sorted({int(m.group(1)) for m in _MARKER.finditer(symbols)})
  if not ids:
    print(f'No ui::AllowlistedResource<> symbols in {args.input}; is it '
          'stripped?', file=sys.stderr)
    return 1
  with open(args.output, 'w') as out:
    out.writelines(f'{i}\n' for i in ids)
  return 0


if __name__ == '__main__':
  sys.exit(main())
