#!/usr/bin/env python

import argparse
import os
import shutil
import subprocess
import sys
import tarfile

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, NODE_VERSION
from lib.util import scoped_cwd, rm_rf, get_atom_shell_version, make_zip, \
                     safe_mkdir


ATOM_SHELL_VRESION = get_atom_shell_version()

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release')
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
  'linux': [
    'atom',
    'libchromiumcontent.so',
    'libffmpegsumo.so',
  ],
}
TARGET_DIRECTORIES = {
  'darwin': [
    'Atom.app',
  ],
  'win32': [
    'resources',
  ],
  'linux': [
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
  'deps/npm',
  'deps/mdb_v8',
]
HEADERS_FILES = [
  'common.gypi',
  'config.gypi',
]


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  args = parse_args()

  force_build()
  if sys.platform != 'linux2':
    download_libchromiumcontent_symbols(args.url)
    create_symbols()
  copy_binaries()
  copy_headers()
  copy_license()
  create_version()
  create_dist_zip()
  create_symbols_zip()
  create_header_tarball()


def parse_args():
  parser = argparse.ArgumentParser(description='Create distributions')
  parser.add_argument('-u', '--url',
                      help='The base URL from which to download '
                      'libchromiumcontent (i.e., the URL you passed to '
                      'libchromiumcontent\'s script/upload script',
                      default=BASE_URL,
                      required=False)
  return parser.parse_args()


def force_build():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_call([sys.executable, build, '-c', 'Release'])


def copy_binaries():
  for binary in TARGET_BINARIES[TARGET_PLATFORM]:
    shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[TARGET_PLATFORM]:
    shutil.copytree(os.path.join(OUT_DIR, directory),
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


def download_libchromiumcontent_symbols(url):
  if sys.platform == 'darwin':
    symbols_name = 'libchromiumcontent.dylib.dSYM'
  elif sys.platform == 'win32':
    symbols_name = 'chromiumcontent.dll.pdb'

  brightray_dir = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor')
  target_dir = os.path.join(brightray_dir, 'download', 'libchromiumcontent')
  symbols_path = os.path.join(target_dir, 'Release', symbols_name)
  if os.path.exists(symbols_path):
    return

  download = os.path.join(brightray_dir, 'libchromiumcontent', 'script',
                          'download')
  subprocess.check_call([sys.executable, download, '-f', '-s', '-c',
                         LIBCHROMIUMCONTENT_COMMIT, url, target_dir])


def create_symbols():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_output([sys.executable, build, '-c', 'Release',
                           '-t', 'atom_dump_symbols'])

  directory = 'Atom-Shell.breakpad.syms'
  shutil.copytree(os.path.join(OUT_DIR, directory),
                  os.path.join(DIST_DIR, directory),
                  symlinks=True)


def create_dist_zip():
  dist_name = 'atom-shell-{0}-{1}.zip'.format(ATOM_SHELL_VRESION,
                                              TARGET_PLATFORM)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[TARGET_PLATFORM] +  ['LICENSE', 'version']
    dirs = TARGET_DIRECTORIES[TARGET_PLATFORM]
    make_zip(zip_file, files, dirs)


def create_symbols_zip():
  dist_name = 'atom-shell-{0}-{1}-symbols.zip'.format(ATOM_SHELL_VRESION,
                                                      TARGET_PLATFORM)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE', 'version']
    dirs = ['Atom-Shell.breakpad.syms']
    if sys.platform == 'linux2':
      dirs = []
    make_zip(zip_file, files, dirs)


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
