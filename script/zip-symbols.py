#!/usr/bin/env python

import glob
import os
import sys

from lib.config import PLATFORM, get_target_arch
from lib.util import scoped_cwd, get_electron_version, make_zip, \
                     get_electron_branding, get_out_dir

ELECTRON_VERSION = get_electron_version()
PROJECT_NAME = get_electron_branding()['project_name']
OUT_DIR = get_out_dir()

def main():
  print('Zipping Symbols')
  if get_target_arch() == 'mips64el':
    return

  dist_name = 'symbols.zip'
  zip_file = os.path.join(OUT_DIR, dist_name)
  licenses = ['LICENSE', 'LICENSES.chromium.html', 'version']

  with scoped_cwd(OUT_DIR):
    dirs = ['{0}.breakpad.syms'.format(PROJECT_NAME)]
    print('Making symbol zip: ' + zip_file)
    make_zip(zip_file, licenses, dirs)

  if PLATFORM == 'darwin':
    dsym_name = 'dsym.zip'
    with scoped_cwd(OUT_DIR):
      dsyms = glob.glob('*.dSYM')
      dsym_zip_file = os.path.join(OUT_DIR, dsym_name)
      print('Making dsym zip: ' + dsym_zip_file)
      make_zip(dsym_zip_file, licenses, dsyms)
  elif PLATFORM == 'win32':
    pdb_name = 'pdb.zip'
    with scoped_cwd(OUT_DIR):
      pdbs = glob.glob('*.pdb')
      pdb_zip_file = os.path.join(OUT_DIR, pdb_name)
      print('Making pdb zip: ' + pdb_zip_file)
      make_zip(pdb_zip_file, pdbs + licenses, [])

if __name__ == '__main__':
  sys.exit(main())