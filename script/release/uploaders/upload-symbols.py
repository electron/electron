#!/usr/bin/env python

import os
import glob
import sys

sys.path.append(
  os.path.abspath(os.path.dirname(os.path.abspath(__file__)) + "/../.."))

from lib.config import PLATFORM, s3_config, enable_verbose_mode
from lib.util import get_electron_branding, execute, rm_rf, safe_mkdir, s3put, \
                     get_out_dir, ELECTRON_DIR

RELEASE_DIR = get_out_dir()


PROJECT_NAME = get_electron_branding()['project_name']
PRODUCT_NAME = get_electron_branding()['product_name']
SYMBOLS_DIR = os.path.join(RELEASE_DIR, 'breakpad_symbols')

PDB_LIST = [
  os.path.join(RELEASE_DIR, '{0}.exe.pdb'.format(PROJECT_NAME))
]


def main():
  os.chdir(ELECTRON_DIR)
  if PLATFORM == 'win32':
    for pdb in PDB_LIST:
      run_symstore(pdb, SYMBOLS_DIR, PRODUCT_NAME)
    files = glob.glob(SYMBOLS_DIR + '/*.pdb/*/*.pdb')
  else:
    files = glob.glob(SYMBOLS_DIR + '/*/*/*.sym') + glob.glob(SYMBOLS_DIR + '/*/*/*.src.zip')

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
