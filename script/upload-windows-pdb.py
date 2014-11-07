#!/usr/bin/env python

import os

from lib.util import execute, rm_rf, safe_mkdir


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SYMBOLS_DIR = 'dist\\symbols'
PDB_LIST = [
  'out\\Release\\atom.exe.pdb',
  'vendor\\brightray\\vendor\\download\\libchromiumcontent\\Release\\chromiumcontent.dll.pdb',
]


def main():
  os.chdir(SOURCE_ROOT)

  rm_rf(SYMBOLS_DIR)
  safe_mkdir(SYMBOLS_DIR)
  for pdb in PDB_LIST:
    run_symstore(pdb, SYMBOLS_DIR, 'AtomShell')


def run_symstore(pdb, dest, product):
  execute(['symstore', 'add', '/r', '/f', pdb, '/s', dest, '/t', product])


if __name__ == '__main__':
  import sys
  sys.exit(main())
