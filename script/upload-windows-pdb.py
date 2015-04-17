#!/usr/bin/env python

import os
import glob
import sys

from lib.config import s3_config
from lib.util import atom_gyp, execute, rm_rf, safe_mkdir, s3put


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
SYMBOLS_DIR = 'dist\\symbols'
DOWNLOAD_DIR = 'vendor\\brightray\\vendor\\download\\libchromiumcontent'

PROJECT_NAME = atom_gyp()['project_name%']
PRODUCT_NAME = atom_gyp()['product_name%']

PDB_LIST = [
  'out\\R\\{0}.exe.pdb'.format(PROJECT_NAME),
  'out\\R\\node.dll.pdb',
]


def main():
  os.chdir(SOURCE_ROOT)

  rm_rf(SYMBOLS_DIR)
  safe_mkdir(SYMBOLS_DIR)
  for pdb in PDB_LIST:
    run_symstore(pdb, SYMBOLS_DIR, PRODUCT_NAME)

  bucket, access_key, secret_key = s3_config()
  files = glob.glob(SYMBOLS_DIR + '/*.pdb/*/*.pdb')
  files = [f.lower() for f in files]
  upload_symbols(bucket, access_key, secret_key, files)


def run_symstore(pdb, dest, product):
  execute(['symstore', 'add', '/r', '/f', pdb, '/s', dest, '/t', product])


def upload_symbols(bucket, access_key, secret_key, files):
  s3put(bucket, access_key, secret_key, SYMBOLS_DIR, 'atom-shell/symbols',
        files)


if __name__ == '__main__':
  sys.exit(main())
