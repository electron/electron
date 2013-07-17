#!/usr/bin/env python

import errno
import glob
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

  force_build()
  copy_binaries()
  copy_license()
  create_version()
  create_zip()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_call([sys.executable, build, '-c', 'Release']);


def copy_binaries():
  shutil.copytree(BUNDLE_DIR, os.path.join(DIST_DIR, BUNDLE_NAME),
                  symlinks=True)


def copy_license():
  license = os.path.join(SOURCE_ROOT, 'LICENSE')
  shutil.copy2(license, DIST_DIR)


def create_version():
  commit = subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(commit)


def create_zip():
  print "Zipping distribution..."
  zip_file = os.path.join(SOURCE_ROOT, 'atom-shell.zip')
  safe_unlink(zip_file)

  with scoped_cwd(DIST_DIR):
    files = glob.glob('*')
    subprocess.check_call(['zip', '-r', '-y', zip_file] + files)


if __name__ == '__main__':
  sys.exit(main())
