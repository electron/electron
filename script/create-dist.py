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
NODE_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'node')
DIST_HEADERS_DIR = os.path.join(DIST_DIR, 'node-{0}'.format(get_atom_shell_version()))

BUNDLE_NAME = 'Atom.app'
BUNDLE_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release', BUNDLE_NAME)

HEADERS_SUFFIX = [
  '.h',
  '.gypi',
]
HEADERS_DIRS = [
  'src',
  'deps/http_parser',
  'deps/v8',
  'deps/zlib',
  'deps/uv',
  'tools',
]
HEADERS_FILES = [
]


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  force_build()
  copy_binaries()
  copy_headers()
  copy_license()
  create_version()
  create_zip()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_call([sys.executable, build, '-c', 'Release']);


def copy_binaries():
  shutil.copytree(BUNDLE_DIR, os.path.join(DIST_DIR, BUNDLE_NAME),
                  symlinks=True)


def copy_headers():
  os.mkdir(DIST_HEADERS_DIR)
  for include_path in HEADERS_DIRS:
    abs_path = os.path.join(NODE_DIR, include_path)
    for dirpath, dirnames, filenames in os.walk(abs_path):
      for filename in filenames:
        extension = os.path.splitext(filename)[1]
        if extension not in HEADERS_SUFFIX:
          continue
        copy_source_file(os.path.join(dirpath, filename))


def copy_license():
  license = os.path.join(SOURCE_ROOT, 'LICENSE')
  shutil.copy2(license, DIST_DIR)


def create_version():
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(get_atom_shell_version())


def create_zip():
  print "Zipping distribution..."
  zip_file = os.path.join(SOURCE_ROOT, 'atom-shell.zip')
  safe_unlink(zip_file)

  with scoped_cwd(DIST_DIR):
    files = ['Atom.app', 'LICENSE', 'version']
    subprocess.check_call(['zip', '-r', '-y', zip_file] + files)


def copy_source_file(source):
  relative = os.path.relpath(source, start=NODE_DIR)
  destination = os.path.join(DIST_HEADERS_DIR, relative)
  safe_mkdir(os.path.dirname(destination))
  shutil.copy2(source, destination)


if __name__ == '__main__':
  sys.exit(main())
