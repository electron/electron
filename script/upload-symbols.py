#!/usr/bin/env python

import os
import glob
import sys

from lib.config import PLATFORM, s3_config, enable_verbose_mode
from lib.util import electron_gyp, execute, rm_rf, safe_mkdir, s3put


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
RELEASE_DIR = os.path.join(SOURCE_ROOT, 'out', 'R')

PROJECT_NAME = electron_gyp()['project_name%']
PRODUCT_NAME = electron_gyp()['product_name%']

if PLATFORM == 'win32':
  SYMBOLS_DIR = os.path.join(DIST_DIR, 'symbols')
else:
  SYMBOLS_DIR = os.path.join(DIST_DIR, '{0}.breakpad.syms'.format(PROJECT_NAME))

PDB_LIST = [
  os.path.join(RELEASE_DIR, '{0}.exe.pdb'.format(PROJECT_NAME)),
  os.path.join(RELEASE_DIR, 'node.dll.pdb')
]


def main():
  os.chdir(SOURCE_ROOT)
  if PLATFORM == 'win32':
    for pdb in PDB_LIST:
      run_symstore(pdb, SYMBOLS_DIR, PRODUCT_NAME)
    files = glob.glob(SYMBOLS_DIR + '/*.pdb/*/*.pdb')
  else:
    files = glob.glob(SYMBOLS_DIR + '/*/*/*.sym')

  # The file upload needs to be atom-shell/symbols/:symbol_name/:hash/:symbol
  os.chdir(SYMBOLS_DIR)
  files = [os.path.relpath(f, os.getcwd()) for f in files]

  # The symbol server needs lowercase paths, it will fail otherwise
  # So lowercase all the file paths here
  files = [f.lower() for f in files]

  bucket, access_key, secret_key = s3_config()
  upload_symbols(bucket, access_key, secret_key, files)


def run_symstore(pdb, dest, product):
  execute(['symstore', 'add', '/r', '/f', pdb, '/s', dest, '/t', product])


def upload_symbols(bucket, access_key, secret_key, files):
  s3put(bucket, access_key, secret_key, SYMBOLS_DIR, 'atom-shell/symbols',
        files)


if __name__ == '__main__':
  sys.exit(main())
