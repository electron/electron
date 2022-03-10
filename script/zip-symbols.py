#!/usr/bin/env python

from __future__ import print_function
import argparse
import glob
import os
import sys

from lib.config import PLATFORM, get_target_arch
from lib.util import scoped_cwd, get_electron_version, make_zip, \
                     get_electron_branding, get_out_dir, execute

ELECTRON_VERSION = get_electron_version()
PROJECT_NAME = get_electron_branding()['project_name']
OUT_DIR = get_out_dir()

def main():
  print('Zipping Symbols')
  if get_target_arch() == 'mips64el':
    return

  args = parse_args()
  dist_name = 'symbols.zip'
  zip_file = os.path.join(args.build_dir, dist_name)
  licenses = ['LICENSE', 'LICENSES.chromium.html', 'version']

  with scoped_cwd(args.build_dir):
    dirs = ['breakpad_symbols']
    print('Making symbol zip: ' + zip_file)
    make_zip(zip_file, licenses, dirs)

  if PLATFORM == 'darwin':
    dsym_name = 'dsym.zip'
    with scoped_cwd(args.build_dir):
      dsyms = glob.glob('*.dSYM')
      snapshot_dsyms = ['v8_context_snapshot_generator.dSYM']
      for dsym in snapshot_dsyms:
        if (dsym in dsyms):
          dsyms.remove(dsym)
      dsym_zip_file = os.path.join(args.build_dir, dsym_name)
      print('Making dsym zip: ' + dsym_zip_file)
      make_zip(dsym_zip_file, licenses, dsyms)
      dsym_snapshot_name = 'dsym-snapshot.zip'
      dsym_snapshot_zip_file = os.path.join(args.build_dir, dsym_snapshot_name)
      print('Making dsym snapshot zip: ' + dsym_snapshot_zip_file)
      make_zip(dsym_snapshot_zip_file, licenses, snapshot_dsyms)
      if len(dsyms) > 0 and 'DELETE_DSYMS_AFTER_ZIP' in os.environ:
        execute(['rm', '-rf'] + dsyms)
  elif PLATFORM == 'win32':
    pdb_name = 'pdb.zip'
    with scoped_cwd(args.build_dir):
      pdbs = glob.glob('*.pdb')
      pdb_zip_file = os.path.join(args.build_dir, pdb_name)
      print('Making pdb zip: ' + pdb_zip_file)
      make_zip(pdb_zip_file, pdbs + licenses, [])
  elif PLATFORM == 'linux':
    debug_name = 'debug.zip'
    with scoped_cwd(args.build_dir):
      dirs = ['debug']
      debug_zip_file = os.path.join(args.build_dir, debug_name)
      print('Making debug zip: ' + debug_zip_file)
      make_zip(debug_zip_file, licenses, dirs)

def parse_args():
  parser = argparse.ArgumentParser(description='Zip symbols')
  parser.add_argument('-b', '--build-dir',
                      help='Path to an Electron build folder.',
                      default=OUT_DIR,
                      required=False)
  return parser.parse_args()

if __name__ == '__main__':
  sys.exit(main())
