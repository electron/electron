#!/usr/bin/env python

import argparse
import os
import re
import shutil
import subprocess
import sys

from lib.config import LIBCHROMIUMCONTENT_COMMIT, BASE_URL, TARGET_PLATFORM, \
                       DIST_ARCH
from lib.util import scoped_cwd, rm_rf, get_atom_shell_version, make_zip, \
                     execute, get_chromedriver_version


ATOM_SHELL_VERSION = get_atom_shell_version()

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
DIST_DIR = os.path.join(SOURCE_ROOT, 'dist')
OUT_DIR = os.path.join(SOURCE_ROOT, 'out', 'Release')

SYMBOL_NAME = {
  'darwin': 'libchromiumcontent.dylib.dSYM',
  'linux': 'libchromiumcontent.so.dbg',
  'win32': 'chromiumcontent.dll.pdb',
}[TARGET_PLATFORM]

TARGET_BINARIES = {
  'darwin': [
  ],
  'win32': [
    'atom.exe',
    'chromiumcontent.dll',
    'content_shell.pak',
    'd3dcompiler_46.dll',
    'ffmpegsumo.dll',
    'icudtl.dat',
    'libEGL.dll',
    'libGLESv2.dll',
    'msvcp120.dll',
    'msvcr120.dll',
    'content_resources_200_percent.pak',
    'ui_resources_200_percent.pak',
    'vccorlib120.dll',
    'xinput1_3.dll',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
  'linux': [
    'atom',
    'content_shell.pak',
    'icudtl.dat',
    'libchromiumcontent.so',
    'libffmpegsumo.so',
    'natives_blob.bin',
    'snapshot_blob.bin',
  ],
}
TARGET_DIRECTORIES = {
  'darwin': [
    'Atom.app',
  ],
  'win32': [
    'resources',
    'locales',
  ],
  'linux': [
    'resources',
    'locales',
  ],
}

SYSTEM_LIBRARIES = [
  'libgcrypt.so',
  'libnotify.so',
]


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  args = parse_args()

  force_build()
  download_libchromiumcontent_symbols(args.url)
  create_symbols()
  copy_binaries()
  copy_chromedriver()
  copy_license()

  if TARGET_PLATFORM == 'linux':
    copy_system_libraries()

  create_version()
  create_dist_zip()
  create_chromedriver_zip()
  create_symbols_zip()


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
  execute([sys.executable, build, '-c', 'Release'])


def copy_binaries():
  for binary in TARGET_BINARIES[TARGET_PLATFORM]:
    shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[TARGET_PLATFORM]:
    shutil.copytree(os.path.join(OUT_DIR, directory),
                    os.path.join(DIST_DIR, directory),
                    symlinks=True)


def copy_chromedriver():
  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  execute([sys.executable, build, '-c', 'Release', '-t', 'copy_chromedriver'])
  binary = 'chromedriver'
  if TARGET_PLATFORM == 'win32':
    binary += '.exe'
  shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)


def copy_license():
  shutil.copy2(os.path.join(SOURCE_ROOT, 'LICENSE'), DIST_DIR)


def copy_system_libraries():
  ldd = execute(['ldd', os.path.join(OUT_DIR, 'atom')])
  lib_re = re.compile('\t(.*) => (.+) \(.*\)$')
  for line in ldd.splitlines():
    m = lib_re.match(line)
    if not m:
      continue
    for i, library in enumerate(SYSTEM_LIBRARIES):
      real_library = m.group(1)
      if real_library.startswith(library):
        shutil.copyfile(m.group(2), os.path.join(DIST_DIR, real_library))
        SYSTEM_LIBRARIES[i] = real_library


def create_version():
  version_path = os.path.join(SOURCE_ROOT, 'dist', 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(ATOM_SHELL_VERSION)


def download_libchromiumcontent_symbols(url):
  brightray_dir = os.path.join(SOURCE_ROOT, 'vendor', 'brightray', 'vendor')
  target_dir = os.path.join(brightray_dir, 'download', 'libchromiumcontent')
  symbols_path = os.path.join(target_dir, 'Release', SYMBOL_NAME)
  if os.path.exists(symbols_path):
    return

  download = os.path.join(brightray_dir, 'libchromiumcontent', 'script',
                          'download')
  subprocess.check_call([sys.executable, download, '-f', '-s', '-c',
                         LIBCHROMIUMCONTENT_COMMIT, url, target_dir])


def create_symbols():
  directory = 'Atom-Shell.breakpad.syms'
  rm_rf(os.path.join(OUT_DIR, directory))

  build = os.path.join(SOURCE_ROOT, 'script', 'build.py')
  subprocess.check_output([sys.executable, build, '-c', 'Release',
                           '-t', 'atom_dump_symbols'])

  shutil.copytree(os.path.join(OUT_DIR, directory),
                  os.path.join(DIST_DIR, directory),
                  symlinks=True)


def create_dist_zip():
  dist_name = 'atom-shell-{0}-{1}-{2}.zip'.format(ATOM_SHELL_VERSION,
                                                  TARGET_PLATFORM, DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[TARGET_PLATFORM] +  ['LICENSE', 'version']
    if TARGET_PLATFORM == 'linux':
      files += SYSTEM_LIBRARIES
    dirs = TARGET_DIRECTORIES[TARGET_PLATFORM]
    make_zip(zip_file, files, dirs)


def create_chromedriver_zip():
  dist_name = 'chromedriver-{0}-{1}-{2}.zip'.format(get_chromedriver_version(),
                                                    TARGET_PLATFORM, DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE']
    if TARGET_PLATFORM == 'win32':
      files += ['chromedriver.exe']
    else:
      files += ['chromedriver']
    make_zip(zip_file, files, [])


def create_symbols_zip():
  dist_name = 'atom-shell-{0}-{1}-{2}-symbols.zip'.format(ATOM_SHELL_VERSION,
                                                          TARGET_PLATFORM,
                                                          DIST_ARCH)
  zip_file = os.path.join(SOURCE_ROOT, 'dist', dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE', 'version']
    dirs = ['Atom-Shell.breakpad.syms']
    make_zip(zip_file, files, dirs)


if __name__ == '__main__':
  sys.exit(main())
