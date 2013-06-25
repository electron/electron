#!/usr/bin/env python

import errno
import os
import shutil
import subprocess
import sys

from lib.util import *


SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
BUNDLE_NAME = 'Atom.app'
BUNDLE_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release', BUNDLE_NAME)


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  copy_binaries()
  create_zip()


def copy_binaries():
  shutil.copytree(BUNDLE_DIR, os.path.join(DIST_DIR, BUNDLE_NAME),
                  symlinks=True)


def create_zip():
  print "Zipping distribution..."
  zip_file = os.path.join(SOURCE_ROOT, 'atom-shell.zip')
  safe_unlink(zip_file)

  cwd = os.getcwd()
  os.chdir(DIST_DIR)
  subprocess.check_call(['zip', '-r', '-y', zip_file, 'Atom.app'])
  os.chdir(cwd)


if __name__ == '__main__':
  sys.exit(main())
