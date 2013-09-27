#!/usr/bin/env python

import os
import shutil
import subprocess
import sys
import tarfile

from lib.util import scoped_cwd, rm_rf, get_atom_shell_version, make_zip, \
                     safe_mkdir


ATOM_SHELL_VRESION = get_atom_shell_version()
NODE_VERSION = 'v0.10.18'

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
NODE_DIR = os.path.join(SOURCE_ROOT, 'vendor', 'node')
DIST_HEADERS_NAME = 'node-{0}'.format(NODE_VERSION)
DIST_HEADERS_DIR = os.path.join(DIST_DIR, DIST_HEADERS_NAME)

TARGET_PLATFORM = {
  'cygwin': 'win32',
  'darwin': 'darwin',
  'linux2': 'linux',
  'win32': 'win32',
}[sys.platform]

TARGET_BINARIES = {
  'darwin': [
  ],
  'win32': [
    'atom.exe',
    'chromiumcontent.dll',
    'content_shell.pak',
    'ffmpegsumo.dll',
    'icudt.dll',
    'libGLESv2.dll',
  ],
}
TARGET_DIRECTORIES = {
  'darwin': [
    'Atom.app',
  ],
  'win32': [
    'resources',
  ],
}

HEADERS_SUFFIX = [
  '.h',
  '.gypi',
]
HEADERS_DIRS = [
  'src',
  'deps/http_parser',
  'deps/zlib',
  'deps/uv',
]
HEADERS_FILES = [
  'common.gypi',
  'config.gypi',
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
  create_header_tarball()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_call([sys.executable, build, '-c', 'Release'])


def copy_binaries():
  out_dir = os.path.join(SOURCE_ROOT, 'out', 'Release')

  for binary in TARGET_BINARIES[TARGET_PLATFORM]:
    shutil.copy2(os.path.join(out_dir, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[TARGET_PLATFORM]:
    shutil.copytree(os.path.join(out_dir, directory),
                    os.path.join(DIST_DIR, directory),
                    symlinks=True)


def copy_headers():
  os.mkdir(DIST_HEADERS_DIR)
  # Copy standard node headers from node. repository.
  for include_path in HEADERS_DIRS:
    abs_path = os.path.join(NODE_DIR, include_path)
    for dirpath, _, filenames in os.walk(abs_path):
      for filename in filenames:
        extension = os.path.splitext(filename)[1]
        if extension not in HEADERS_SUFFIX:
          continue
        copy_source_file(os.path.join(dirpath, filename))
  for other_file in HEADERS_FILES:
    copy_source_file(source = os.path.join(NODE_DIR, other_file))

  # Copy V8 headers from chromium's repository.
  src = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor', 'download',
                    'libchromiumcontent', 'src')
  for dirpath, _, filenames in os.walk(os.path.join(src, 'v8')):
    for filename in filenames:
      extension = os.path.splitext(filename)[1]
      if extension not in HEADERS_SUFFIX:
        continue
      copy_source_file(source=os.path.join(dirpath, filename),
                       start=src,
                       destination=os.path.join(DIST_HEADERS_DIR, 'deps'))


def copy_license():
  shutil.copy2(os.path.join(SOURCE_ROOT, 'LICENSE'), DIST_DIR)


def create_version():
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(ATOM_SHELL_VRESION)


def create_zip():
  dist_name = 'atom-shell-{0}-{1}.zip'.format(ATOM_SHELL_VRESION,
                                              TARGET_PLATFORM)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[TARGET_PLATFORM] +  \
            TARGET_DIRECTORIES[TARGET_PLATFORM] + \
            ['LICENSE', 'version']
    make_zip(zip_file, files)


def create_header_tarball():
  with scoped_cwd(DIST_DIR):
    tarball = tarfile.open(name=DIST_HEADERS_DIR + '.tar.gz', mode='w:gz')
    tarball.add(DIST_HEADERS_NAME)
    tarball.close()


def copy_source_file(source, start=NODE_DIR, destination=DIST_HEADERS_DIR):
  relative = os.path.relpath(source, start=start)
  final_destination = os.path.join(destination, relative)
  safe_mkdir(os.path.dirname(final_destination))
  shutil.copy2(source, final_destination)


if __name__ == '__main__':
  sys.exit(main())
